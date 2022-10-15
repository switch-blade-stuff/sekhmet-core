/*
 * Created by switchblade on 12/05/22
 */

#pragma once

#include <filesystem>
#include <vector>

#include "expected.hpp"

namespace sek
{
	namespace detail
	{
		using module_path_char = typename std::filesystem::path::value_type;
		using module_handle = void *;

		expected<module_handle, std::error_code> open_module(const module_path_char *);
		expected<void, std::error_code> close_module(module_handle);

		struct module_data;
		struct module_db;
	}	 // namespace detail

	/** @brief Handle used to reference a native dynamic library which contains one or multiple plugins. */
	class module
	{
	public:
		typedef detail::module_handle native_handle_type;

	private:
		using path_char = detail::module_path_char;
		using data_t = detail::module_data;

	public:
		/** @brief Loads a module.
		 * @param path Path to the module library.
		 * @return Handle to the loaded module.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once. */
		static module open(const std::filesystem::path &path) { return open(path.c_str()); }
		/** @copydoc open
		 * @note If the module path is `nullptr`, returns handle to the main (parent executable) module. */
		static SEK_CORE_PUBLIC module open(const path_char *path);

		/** @copybrief open
		 * @param path Path to the module library.
		 * @return Handle to the loaded module, or an error code.
		 * @note A module library is guaranteed to be loaded only once. */
		static expected<module, std::error_code> open(std::nothrow_t, const std::filesystem::path &path)
		{
			return open(std::nothrow, path.c_str());
		}
		/** @copydoc open
		 * @note If the module path is `nullptr`, returns handle to the main (parent executable) module. */
		static SEK_CORE_PUBLIC expected<module, std::error_code> open(std::nothrow_t, const path_char *path);

		/** @brief Closes an open (loaded) module handle. If the internal reference counter reaches 0, unloads the module.
		 * @param mod Handle to a previously loaded module.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note If the module is the main (parent executable) module, does nothing. */
		static SEK_CORE_PUBLIC void close(module &mod);
		/** @copybrief close
		 * @param mod Handle to a previously loaded module.
		 * @return `void` or an error code.
		 * @note If the module is the main (parent executable) module, does nothing. */
		static SEK_CORE_PUBLIC expected<void, std::error_code> close(std::nothrow_t, module &mod);

	private:
		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

		constexpr explicit module(data_t *data) noexcept;

	public:
		module(const module &) = delete;
		module &operator=(const module &) = delete;

		/** Opens a module handle referencing the main (parent executable) module. */
		SEK_CORE_PUBLIC module();
		/** Closes the module handle. */
		SEK_CORE_PUBLIC ~module();

		constexpr module(module &&other) noexcept { swap(other); }
		constexpr module &operator=(module &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Loads a module.
		 * @param path Path to the module library.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once.
		 * @note If the module path is `nullptr`, initializes a main (parent executable) module handle. */
		SEK_CORE_PUBLIC explicit module(const path_char *path);
		/** @copydoc native_file */
		explicit module(const std::filesystem::path &path) : module(path.c_str()) {}

		/** Checks if the module handle is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return m_data != nullptr; }
		/** @copydoc is_open */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns the underlying native library handle. */
		[[nodiscard]] SEK_CORE_PUBLIC native_handle_type native_handle() const noexcept;

		constexpr void swap(module &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(module &a, module &b) noexcept { a.swap(b); }

	protected:
		data_t *m_data = nullptr;
	};
}	 // namespace sek