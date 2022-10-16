/*
 * Created by switchblade on 28/04/22
 */

#include "../plugin.hpp"

#include <cstdlib>

#include "../access_guard.hpp"
#include "../debug/assert.hpp"
#include "../debug/logger.hpp"
#include "../detail/sparse_hash_table.hpp"

#if defined(SEK_OS_WIN)
#include <errhandlingapi.h>
#include <libloaderapi.h>

#define PATH_MAX MAX_PATH

#elif defined(SEK_OS_UNIX)
#include <dlfcn.h>
#if defined(SEK_OS_APPLE)
#include <mach-o/dyld.h>
#endif
#else
#error "Unknown OS"
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
			module_data(module_handle handle, const module_path_char *path) : handle(handle), path(path) {}
			module_data(module_handle handle, std::filesystem::path &&path) : handle(handle), path(std::move(path)) {}

			module_handle handle;

			std::filesystem::path path;
			std::size_t ref_ctr = 0;
		};
		struct module_db
		{
			using alloc_t = std::allocator<module_data>;
			using cmp_t = std::equal_to<>;
			using hash_t = default_hash;

			struct key_get
			{
				constexpr auto operator()(const module_data &data) const noexcept { return data.handle; }
			};

			using data_table = sparse_hash_table<module_handle, module_data, hash_t, cmp_t, key_get, alloc_t>;

			[[nodiscard]] static expected<module_handle, std::error_code> load(const module_path_char *path) noexcept
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
					return unexpected{make_errc()};
#elif defined(SEK_OS_UNIX)
				if ((res = dlopen(path, RTLD_NOW | RTLD_GLOBAL)) == nullptr) [[unlikely]]
					return unexpected{current_error()};
#endif
				return res;
			}
			[[nodiscard]] static expected<void, std::error_code> unload(module_handle handle) noexcept
			{
#if defined(SEK_OS_WIN)
				if (!FreeLibrary(handle)) [[unlikely]]
					return unexpected{get_error_code()};
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
						const auto code = get_error_code();
						if (code == 0x7A /* ERROR_INSUFFICIENT_BUFFER */) [[likely]]
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
				const auto res = load(nullptr);
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

			[[nodiscard]] static access_guard<module_db> instance()
			{
				static module_db instance;
				return {instance, instance.mtx};
			}

			module_db() { main = table.emplace(main_lib(), main_path()).first.get(); }

			expected<module_data *, std::error_code> open(const module_path_char *path)
			{
				module_data *result;
				if (path == nullptr) [[unlikely]]
					result = main;
				else
				{
					const auto handle = load(path);
					if (handle.has_value()) [[unlikely]]
						return unexpected{handle.error()};

					/* If such entry does not exist yet, create it. */
					auto iter = table.find(*handle);
					if (iter == table.end()) [[likely]]
						iter = table.emplace(*handle, path).first;

					/* Always increment use counter for non-main modules. */
					result = iter.get();
					++result->ref_ctr;
				}
				return result;
			}
			expected<void, std::error_code> close(module_data *data)
			{
				if (data != main) [[likely]]
				{
					/* Erase the table entry if it's reference counter has reached 0. */
					if (--data->ref_ctr == 0) [[unlikely]]
					{
						const auto target = table.find(data->handle);
						SEK_ASSERT(target != table.end(), "Invalid module data entry");
						table.erase(target);
					}
					return unload(data->handle);
				}
			}

			std::mutex mtx;
			data_table table;
			module_data *main;
		};
	}	 // namespace detail

	module module::main() { return module{detail::module_db::instance()->main}; }

	module::~module()
	{
		if (is_open()) close();
	}

	expected<void, std::error_code> module::open(std::nothrow_t, const path_char *path)
	{
		auto result = detail::module_db::instance()->open(path);
		if (result.has_value()) [[likely]]
			m_data = *result;
		return std::move(result);
	}
	expected<void, std::error_code> module::close(std::nothrow_t)
	{
		return detail::module_db::instance()->close(std::exchange(m_data, nullptr));
	}

	void module::open(const path_char *path) { return_if(open(std::nothrow, path)); }
	void module::close() { return_if(close(std::nothrow)); }

	const std::filesystem::path &module::path() const noexcept { return m_data->path; }
	detail::module_handle module::native_handle() const noexcept { return m_data->handle; }

	core_plugin_interface::~core_plugin_interface() = default;
}	 // namespace sek
