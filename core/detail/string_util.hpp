/*
 * Created by switchblade on 2021-10-20
 */

#pragma once

#include <cstring>

#include "../define.h"
#include "../meta.hpp"

namespace sek::detail
{
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length_slow(I str) noexcept
	{
		for (std::size_t i = 0;; ++i)
			if (*str++ == '\0') return i;
	}
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length_slow(I str, std::size_t max) noexcept
	{
		for (std::size_t i = 0;; ++i)
			if (i == max || *str++ == '\0') return i;
	}
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		return str_length_slow(str);
	}
	template<std::forward_iterator I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		return str_length_slow(str, max);
	}
	template<forward_iterator_for<char> I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str);
		else
			return static_cast<std::size_t>(strlen(std::to_address(str)));
	}
	template<forward_iterator_for<char> I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str, max);
		else
			return static_cast<std::size_t>(strnlen(std::to_address(str), static_cast<std::size_t>(max)));
	}
	template<forward_iterator_for<char8_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str);
		else
		{
			auto ptr = reinterpret_cast<const char *>(std::to_address(str));
			return static_cast<std::size_t>(strlen(ptr));
		}
	}
	template<forward_iterator_for<char8_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str, max);
		else
		{
			auto ptr = reinterpret_cast<const char *>(std::to_address(str));
			return static_cast<std::size_t>(strnlen(ptr, static_cast<std::size_t>(max)));
		}
	}
	template<forward_iterator_for<wchar_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str);
		else
			return static_cast<std::size_t>(wcslen(std::to_address(str)));
	}
	template<forward_iterator_for<wchar_t> I>
	[[nodiscard]] constexpr std::size_t str_length(I str, std::size_t max) noexcept
	{
		if (std::is_constant_evaluated())
			return str_length_slow(str, max);
		else
			return static_cast<std::size_t>(wcsnlen(std::to_address(str), static_cast<std::size_t>(max)));
	}
}	 // namespace sek::detail