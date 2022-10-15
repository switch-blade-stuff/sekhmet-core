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
		struct module_data;
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
		/** Returns module handle referencing the main (parent executable) module. */
		[[nodiscard]] static SEK_CORE_PUBLIC module main();

	private:
		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

		constexpr explicit module(data_t *data) noexcept : m_data(data) {}

	public:
		module(const module &) = delete;
		module &operator=(const module &) = delete;

		/** Initializes a closed module handle. */
		constexpr module() noexcept = default;
		/** Closes the module handle if it is open. */
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
		explicit module(const path_char *path) { open(path); }
		/** @copydoc native_file */
		explicit module(const std::filesystem::path &path) : module(path.c_str()) {}

		/** @brief Loads a module.
		 * @param path Path to the module library.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once. */
		void open(const std::filesystem::path &path) { return open(path.c_str()); }
		/** @copydoc open
		 * @note If the module path is `nullptr`, opens a handle to the main (parent executable) module. */
		SEK_CORE_PUBLIC void open(const path_char *path);
		/** @copybrief open
		 * @param path Path to the module library.
		 * @return `void` or an error code.
		 * @note A module library is guaranteed to be loaded only once. */
		expected<void, std::error_code> open(std::nothrow_t, const std::filesystem::path &path)
		{
			return open(std::nothrow, path.c_str());
		}
		/** @copydoc open
		 * @note If the module path is `nullptr`, opens a handle to the main (parent executable) module. */
		SEK_CORE_PUBLIC expected<void, std::error_code> open(std::nothrow_t, const path_char *path);

		/** @brief Closes an open module handle. If the internal reference counter reaches 0, unloads the module.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note If the module handle references the main (parent executable) module, does nothing. */
		SEK_CORE_PUBLIC void close();
		/** @copybrief close
		 * @return `void` or an error code.
		 * @note If the module references the main (parent executable) module, does nothing. */
		SEK_CORE_PUBLIC expected<void, std::error_code> close(std::nothrow_t);

		/** Checks if the module handle is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return m_data != nullptr; }
		/** @copydoc is_open */
		[[nodiscard]] constexpr operator bool() const noexcept { return is_open(); }

		/** Returns the underlying native library handle. */
		[[nodiscard]] SEK_CORE_PUBLIC native_handle_type native_handle() const noexcept;
		/** Returns path to the underlying native library. */
		[[nodiscard]] SEK_CORE_PUBLIC const std::filesystem::path &path() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const module &other) const noexcept { return m_data <=> other.m_data; }
		[[nodiscard]] constexpr bool operator==(const module &other) const noexcept { return m_data == other.m_data; }

		constexpr void swap(module &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(module &a, module &b) noexcept { a.swap(b); }

	protected:
		data_t *m_data = nullptr;
	};
}	 // namespace sek