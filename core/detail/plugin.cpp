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
	namespace detail
	{
		[[nodiscard]] static std::error_code get_error_code() noexcept
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
			module_data(const module_path_char *path, module_handle handle) : handle(handle), path(path) {}
			module_data(const std::filesystem::path &path, module_handle handle) : handle(handle), path(path) {}
			module_data(std::filesystem::path &&path, module_handle handle) : handle(handle), path(std::move(path)) {}

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
					const auto code = get_error_code();
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
				const auto res = open_module(nullptr);
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

			module_db() { main = table.emplace(main_path(), main_lib()).first.get(); }

			std::mutex mtx;

			const module_data *main;
			data_table table;
		};

		expected<module_handle, std::error_code> open_module(const module_path_char *path)
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
				return unexpected{get_error_code()};
#endif
			return res;
		}
		expected<void, std::error_code> close_module(module_handle handle)
		{
#if defined(SEK_OS_WIN)
			if (!FreeLibrary(handle)) [[unlikely]]
				return unexpected{get_error_code()};
#elif defined(SEK_OS_UNIX)
			if (dlclose(handle)) [[unlikely]]
				return unexpected{get_error_code()};
#endif
			return {};
		}
	}	 // namespace detail

	constexpr module::module(data_t *data) noexcept : m_data(data) { ++data->ref_ctr; }

	expected<module, std::error_code> module::open(std::nothrow_t, const path_char *path)
	{
		if (path == nullptr) [[unlikely]]
			return module{};

		const auto handle = detail::open_module(path);
		if (!handle.has_value()) [[unlikely]]
			return unexpected{handle.error()};

		const auto db = detail::module_db::instance().access();
		auto iter = db->table.find(*handle);
		if (iter == db->table.end()) [[likely]]
			iter = db->table.emplace(path, *handle).first;
		return module{iter.get()};
	}
	expected<void, std::error_code> module::close(std::nothrow_t, module &mod)
	{
		if (const auto data = mod.m_data; data != nullptr) [[likely]]
			if (const auto db = detail::module_db::instance().access(); db->main != data) [[likely]]
			{
				if (--data->ref_ctr == 0) [[unlikely]]
					db->table.erase(data->handle);
				return native_close(data->handle);
			}
		return {};
	}

	module module::open(const module_path_char *path) { return return_if(open(std::nothrow, path)); }
	void module::close(sek::module &mod) { return_if(close(std::nothrow, mod)); }

	module::module() :m_data(module_db::instance()->main) {}
	module::module(const module_path_char *path)
	{
		if (const auto db = module_db::instance().access(); path == nullptr) [[unlikely]]
			m_data = db->main;
		else
		{
			const auto handle = native_open(path);
			if (!handle.has_value()) [[unlikely]]
				throw std::system_error(handle.error());

			auto iter = db->table.find(*handle);
			if (iter == db->table.end()) [[likely]]
				iter = db->table.emplace(path, *handle).first;

			m_data = iter.get();
			m_data->ref_ctr++;
		}
	}
	module::~module() { close(*this); }

	typename module::native_module_handleype module::native_handle() const noexcept { return m_data->handle; }
}	 // namespace sek
