/*
 * Created by switchblade on 28/04/22
 */

#include "../plugin.hpp"

#include "../access_guard.hpp"
#include "../debug/assert.hpp"
#include "../debug/logger.hpp"
#include "../sparse_set.hpp"

#if defined(SEK_OS_WIN)
#include <cstdlib>
#include <errhandlingapi.h>
#include <libloaderapi.h>
#elif defined(SEK_OS_UNIX)
#include <dlfcn.h>
#if defined(SEK_OS_APPLE)
#include <mach-o/dyld.h>
#endif
#endif

namespace sek
{
	struct module::module_data
	{
		module_data(const path_char *path, native_handle_type handle) : path(path), handle(handle) {}
		module_data(const std::filesystem::path &path, native_handle_type handle) : path(path), handle(handle) {}
		module_data(std::filesystem::path &&path, native_handle_type handle) : path(std::move(path)), handle(handle) {}

		/* Reference counter used to clean up module data entries. */
		mutable std::size_t ref_ctr = 0;

		std::filesystem::path path;
		native_handle_type handle;
	};
	struct module::module_db
	{
		struct data_hash
		{
			typedef std::true_type is_transparent;

			[[nodiscard]] hash_t operator()(const module_data &data) const noexcept { return operator()(data.handle); }
			[[nodiscard]] hash_t operator()(native_handle_type handle) const noexcept
			{
				const auto ptr = std::bit_cast<std::uintptr_t>(handle);
				return static_cast<hash_t>(ptr);
			}
		};
		struct data_cmp
		{
			typedef std::true_type is_transparent;

			[[nodiscard]] bool operator()(const module_data &a, const module_data &b) const noexcept
			{
				return a.handle == b.handle;
			}
			[[nodiscard]] bool operator()(native_handle_type a, const module_data &b) const noexcept
			{
				return a == b.handle;
			}
			[[nodiscard]] bool operator()(const module_data &a, native_handle_type b) const noexcept
			{
				return a.handle == b;
			}
			[[nodiscard]] bool operator()(native_handle_type a, native_handle_type b) const noexcept { return a == b; }
		};

		using module_table = sparse_set<module_data, data_hash, data_cmp>;

		[[nodiscard]] static std::filesystem::path get_main_path() noexcept
		{
			std::basic_string<path_char> buff;
			buff.resize(MAX_PATH, '\0');
			std::uint32_t n = 0;

#if defined(SEK_OS_WIN)
			for (;;)
				if ((n = GetModuleFileNameW(nullptr, buff.data(), buff.size())) == 0) [[unlikely]]
				{
					const auto err = GetLastError();
					if (err == 0x7A /* ERROR_INSUFFICIENT_BUFFER */) [[likely]]
						buff.resize(buff.size() * 2, '\0');
					else
					{
						// clang-format off
						sek::logger::fatal()->log(fmt::format("Failed to get executable path using `GetModuleFileNameW`. "
															  "Error: {}", err));
						// clang-format on
						std::abort();
					}
				}
#elif defined(SEK_OS_APPLE)
			if (_NSGetExecutablePath(buff.data(), &n) != 0) [[unlikely]]
			{
				sek::logger::fatal()->log(fmt::format("Failed to get executable path using `_NSGetExecutablePath`"));
				std::abort();
			}
#elif defined(SEK_OS_UNIX)
			if ((n = readlink("/proc/self/exe", buff.data(), PATH_MAX)) == 0) [[unlikely]]
			{
				// clang-format off
				const auto code = std::make_error_code(std::errc{errno});
				sek::logger::fatal()->log(fmt::format("Failed to get executable path using `readlink. "
													  "Error: [{}] {}`", code.value(), code.message()));
				std::abort();
				// clang-format on
			}
#else
#error "Unknown OS"
#endif
			return std::filesystem::path{std::basic_string_view{buff.data(), n}};
		}
		[[nodiscard]] static native_handle_type get_main_lib() noexcept
		{
			const auto res = native_open(nullptr);
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

		module_db() { main = table.emplace(get_main_path(), get_main_lib()).first.get(); }

		mutable std::mutex mtx;

		const module_data *main;
		module_table table;
	};

	expected<typename sek::module::native_handle_type, std::error_code> module::native_open(const path_char *path)
	{
		native_handle_type res;
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
		{
			const auto errc = static_cast<int>(GetLastError());
			return unexpected{std::error_code{errc, std::system_category()}};
		}
#elif defined(SEK_OS_UNIX)
		if ((res = dlopen(path, RTLD_NOW | RTLD_GLOBAL)) == nullptr) [[unlikely]]
			return unexpected{std::make_error_code(std::errc{errno})};
#else
#error "Unknown OS"
#endif
		return res;
	}
	expected<void, std::error_code> module::native_close(native_handle_type) {}

	constexpr module::module(const module_data *data) noexcept : m_data(data) { ++data->ref_ctr; }

	expected<module, std::error_code> module::open(std::nothrow_t, const path_char *path)
	{
		if (path == nullptr) [[unlikely]]
			return module{};

		const auto handle = native_open(path);
		if (!handle.has_value()) [[unlikely]]
			return unexpected{handle.error()};

		const auto db = module_db::instance().access();
		auto iter = db->table.find(*handle);
		if (iter == db->table.end()) [[likely]]
			iter = db->table.emplace(path, *handle).first;
		return module{iter.get()};
	}
	expected<void, std::error_code> module::close(std::nothrow_t, module &mod)
	{
		if (const auto data = mod.m_data; data != nullptr) [[likely]]
			if (const auto db = module_db::instance().access(); db->main != data) [[likely]]
			{
				if (--data->ref_ctr == 0) [[unlikely]]
					db->table.erase(data->handle);
				return native_close(data->handle);
			}
		return {};
	}

	module module::open(const path_char *path) { return return_if(open(std::nothrow, path)); }
	void module::close(sek::module &mod) { return_if(close(std::nothrow, mod)); }

	module::module() :m_data(module_db::instance()->main) {}
	module::module(const path_char *path)
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

	typename module::native_handle_type module::native_handle() const noexcept { return m_data->handle; }
}	 // namespace sek
