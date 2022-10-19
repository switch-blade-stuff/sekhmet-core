/*
 * Created by switchblade on 28/04/22
 */

#include "../plugin.hpp"

#include <atomic>

#include "../assert.hpp"
#include "../logger.hpp"
#include "../sparse_map.hpp"

#if defined(SEK_OS_WIN)
#include <errhandlingapi.h>
#include <libloaderapi.h>

#define PATH_MAX MAX_PATH

#elif defined(SEK_OS_UNIX)
#include <dlfcn.h>
#if defined(SEK_OS_APPLE)
#include <mach-o/dyld.h>
#endif
#endif

namespace sek
{
	template class SEK_API_EXPORT plugin_group<core_plugin_interface>;

	namespace detail
	{
		[[nodiscard]] inline static std::error_code current_error() noexcept
		{
#if defined(SEK_OS_WIN)
			const auto errc = static_cast<int>(GetLastError());
			return std::error_code{errc, std::system_category()};
#elif defined(SEK_OS_UNIX)
			return std::make_error_code(std::errc{errno});
#endif
		}

		struct module_data
		{
			module_data() noexcept = default;
			module_data(std::filesystem::path &&path) : path(std::move(path)) {}
			module_data(module_handle handle, std::filesystem::path &&path) : path(std::move(path)), handle(handle) {}

			std::atomic<std::size_t> ref_ctr = 0;

			std::filesystem::path path;
			module_handle handle;
		};
		struct module_db
		{
			struct key_hash
			{
				auto operator()(const std::basic_string_view<module_path_char> &path) const noexcept
				{
					return fnv1a(path.data(), path.size());
				}
				auto operator()(const std::filesystem::path &path) const noexcept
				{
					return operator()(std::basic_string_view{path.c_str()});
				}
			};
			struct key_cmp
			{
				// clang-format off
				bool operator()(const std::basic_string_view<module_path_char> &a, const std::basic_string_view<module_path_char> &b) const noexcept
				{
					return a == b;
				}
				bool operator()(const std::filesystem::path &a, const std::basic_string_view<module_path_char> &b) const noexcept
				{
					return operator()(std::basic_string_view{a.c_str()}, b);
				}
				bool operator()(const std::basic_string_view<module_path_char> &a, const std::filesystem::path &b) const noexcept
				{
					return operator()(a, std::basic_string_view{b.c_str()});
				}
				bool operator()(const std::filesystem::path &a, const std::filesystem::path &b) const noexcept
				{
					return operator()(std::basic_string_view{a.c_str()}, std::basic_string_view{b.c_str()});
				}
				// clang-format on
			};
			struct key_get
			{
				auto operator()(const module_data &data) const noexcept
				{
					return std::basic_string_view{data.path.c_str()};
				}
			};
			using alloc_t = std::allocator<module_data>;

			using data_table = sparse_hash_table<module_handle, module_data, key_hash, key_cmp, key_get, alloc_t>;
			using group_table = sparse_map<std::string_view, group_data>;

			[[nodiscard]] static expected<module_handle, std::error_code> load_native(const module_path_char *path) noexcept
			{
				module_handle res;
#if defined(SEK_OS_WIN)
				if (path != nullptr) [[likely]]
				{
					// clang-format off
				const auto flags = LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
								   LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32 |
								   LOAD_LIBRARY_SEARCH_USER_DIRS;
					// clang-format on
					res = LoadLibraryExW(path, nullptr, flags);
				}
				else
					res = GetModuleHandleW(nullptr);

				if (res == nullptr) [[unlikely]]
					return unexpected{current_error()};
#elif defined(SEK_OS_UNIX)
				if ((res = dlopen(path, RTLD_NOW | RTLD_GLOBAL)) == nullptr) [[unlikely]]
					return unexpected{current_error()};
#endif
				return res;
			}
			[[nodiscard]] static expected<void, std::error_code> unload_native(module_handle handle) noexcept
			{
#if defined(SEK_OS_WIN)
				if (!FreeLibrary(static_cast<HMODULE>(handle))) [[unlikely]]
					return unexpected{current_error()};
#elif defined(SEK_OS_UNIX)
				if (dlclose(handle)) [[unlikely]]
					return unexpected{current_error()};
#endif
				return {};
			}

			[[nodiscard]] static std::filesystem::path main_path() noexcept
			{
				std::basic_string<module_path_char> buff;
				buff.resize(PATH_MAX, '\0');
				std::uint32_t n = 0;

#if defined(SEK_OS_WIN)
				for (;;)
					if ((n = GetModuleFileNameW(nullptr, buff.data(), buff.size())) == 0) [[unlikely]]
					{
						const auto code = current_error();
						if (code.value() == 0x7A /* ERROR_INSUFFICIENT_BUFFER */) [[likely]]
							buff.resize(buff.size() * 2, '\0');
						else
						{
							// clang-format off
							sek::logger::fatal()->log(fmt::format("Failed to get executable path using `GetModuleFileNameW`. "
													  "Error: [{}] {}`", code.value(), code.message()));
							// clang-format on
							std::abort();
						}
					}
#elif defined(SEK_OS_APPLE)
				if (_NSGetExecutablePath(buff.data(), &n) != 0) [[unlikely]]
				{
					sek::logger::fatal()->log(
						fmt::format("Failed to get executable path using `_NSGetExecutablePath`"));
					std::abort();
				}
#elif defined(SEK_OS_UNIX)
				const auto res = readlink("/proc/self/exe", buff.data(), PATH_MAX);
				if (res <= 0) [[unlikely]]
				{
					const auto code = current_error();
					// clang-format off
					sek::logger::fatal()->log(fmt::format("Failed to get executable path using `readlink. "
														  "Error: [{}] {}`", code.value(), code.message()));
					// clang-format on
					std::abort();
				}
				n = static_cast<std::uint32_t>(res);
#endif
				return std::filesystem::path{std::basic_string_view{buff.data(), n}};
			}
			[[nodiscard]] static module_handle main_lib() noexcept
			{
				const auto res = load_native(nullptr);
				if (!res.has_value()) [[unlikely]]
				{
					// clang-format off
					const auto code = res.error();
					sek::logger::fatal()->log(fmt::format("Failed to get executable module handle. Error: [{}] {}`",
														  code.value(), code.message()));
					std::abort();
					// clang-format on
				}
				return *res;
			}

			[[nodiscard]] static recursive_guard<module_db> instance()
			{
				static module_db value;
				return {value, value.mtx};
			}

			module_db()
			{
				main = modules.emplace(main_lib(), main_path()).first.get();
				/* The main module must never be unloaded. */
				main->ref_ctr.store(1, std::memory_order_relaxed);
			}

			expected<module_data *, std::error_code> load(const module_path_char *path)
			{
				if (path == nullptr) [[unlikely]]
					return main;

				/* Convert the path to canonical form for table indexing. */
				std::filesystem::path canonical;
				{
					std::error_code err;
					canonical = std::filesystem::canonical(path, err);
					if (err) [[unlikely]]
						return unexpected{err};
				}

				/* Check if the module is already loaded. */
				auto entry = modules.find(canonical);
				if (entry == modules.end()) [[unlikely]]
				{
					/* No module with the same path exists yet, create an empty entry. */
					entry = modules.emplace(std::move(canonical)).first;

					/* Load the module library. */
					const auto handle = load_native(path);
					if (handle.has_value()) [[unlikely]]
						return unexpected{handle.error()};
					entry->handle = *handle;
				}
				return entry.get();
			}
			expected<void, std::error_code> unload(module_data *data)
			{
				expected<void, std::error_code> result;
				if (data != main) [[likely]]
				{
					/* Unload the module library & erase the module entry. */
					const auto entry = modules.find(data->path);
					SEK_ASSERT(entry != modules.end(), "Invalid module data entry");

					result = unload_native(data->handle);
					modules.erase(entry);
				}
				return result;
			}

			std::recursive_mutex mtx;
			module_data *main;
			data_table modules;
			group_table groups;
		};

		group_data *group_data::instance(std::string_view t) noexcept
		{
			auto db = module_db::instance().access();
			auto iter = db->groups.find(t);
			if (iter == db->groups.end()) [[unlikely]]
				iter = db->groups.emplace(std::piecewise_construct, std::forward_as_tuple(t), std::forward_as_tuple()).first;
			return &iter->second;
		}
		void group_data::register_plugin(plugin_interface *ptr) noexcept
		{
			constexpr auto current_ver = version{SEK_CORE_VERSION};
			const auto plugin_ver = ptr->plugin_ver();
			const auto core_ver = ptr->core_ver();
			const auto name = ptr->name();

			if (current_ver.major != core_ver.major || current_ver.minor > core_ver.minor) [[unlikely]]
			{
				// clang-format off
				logger::error()->log(fmt::format("Failed to register plugin \"{}\". "
												 "Incompatible core version: {}",
												 name, core_ver));
				// clang-format on
			}
			else if (ptr->next || ptr->prev) [[unlikely]]
				logger::error()->log(fmt::format("Failed to register plugin \"{}\". Already registered", name));
			else
			{
				logger::info()->log(fmt::format("Registering plugin \"{}\" ver. {}", name, plugin_ver));
				ptr->link(plugins);
			}
		}
		void group_data::unregister_plugin(plugin_interface *ptr) noexcept
		{
			if (ptr->next || ptr->prev) [[likely]]
			{
				logger::info()->log(fmt::format("Unregistering plugin \"{}\"", ptr->name()));
				ptr->unlink();
			}
		}
	}	 // namespace detail

	module module::main() { return module{detail::module_db::instance()->main}; }
	std::vector<module> module::all()
	{
		auto db = detail::module_db::instance().access();

		std::vector<module> result;
		result.reserve(db->modules.size());
		for (auto &data : db->modules) result.emplace_back(module{&data});
		return result;
	}

	void module::copy_init(const module &other)
	{
		if ((m_data = other.m_data) != nullptr) [[likely]]
			++m_data->ref_ctr;
	}
	module::module(const module &other) { copy_init(other); }
	module &module::operator=(const module &other)
	{
		if (this != &other) [[likely]]
		{
			unload();
			copy_init(other);
		}
		return *this;
	}

	expected<void, std::error_code> module::load(std::nothrow_t, const path_char *path)
	{
		auto result = detail::module_db::instance()->load(path);
		if (result.has_value()) [[likely]]
			(m_data = *result)->ref_ctr++;
		return std::move(result);
	}
	expected<void, std::error_code> module::unload(std::nothrow_t)
	{
		if (const auto data = std::exchange(m_data, nullptr); data && --data->ref_ctr == 0) [[unlikely]]
			return detail::module_db::instance()->unload(data);
		return {};
	}

	void module::load(const path_char *path) { return_if(load(std::nothrow, path)); }
	void module::unload() { return_if(unload(std::nothrow)); }

	const std::filesystem::path &module::path() const noexcept { return m_data->path; }
	detail::module_handle module::native_handle() const noexcept { return m_data->handle; }

	plugin_interface::~plugin_interface() = default;
	void plugin_interface::enable() { m_enabled = true; }
	void plugin_interface::disable() { m_enabled = false; }

	core_plugin_interface::~core_plugin_interface() = default;
}	 // namespace sek