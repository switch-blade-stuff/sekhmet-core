/*
 * Created by switchblade on 23/09/22
 */

#pragma once

#include "static_string.hpp"

namespace sek
{
	namespace detail
	{
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name(basic_static_string<char, N> str) noexcept
		{
			if constexpr (I == Last)
			{
				str[J] = '\0';
				return basic_static_string<char, J + 1>{str};
			}
			else if constexpr (Src.starts_with("struct "))
				return format_type_name<Src, J, I + 7, Last>(basic_static_string<char, N - 7>{str});
			else if constexpr (Src.starts_with("class "))
				return format_type_name<Src, J, I + 6, Last>(basic_static_string<char, N - 6>{str});
			else if constexpr (Src.starts_with("union "))
				return format_type_name<Src, J, I + 6, Last>(basic_static_string<char, N - 6>{str});
			else if constexpr (Src.starts_with("enum "))
				return format_type_name<Src, J, I + 5, Last>(basic_static_string<char, N - 5>{str});
			else if constexpr (Src[I] == ' ' || Src[I] == '\t')
				return format_type_name<Src, J, I + 1, Last>(basic_static_string<char, N - 1>{str});
			else
			{
				str[J] = Src[I];
				return format_type_name<Src, J + 1, I + 1, Last>(str);
			}
		}
		template<basic_static_string Src, std::size_t J, std::size_t I, std::size_t Last, std::size_t N>
		consteval auto format_type_name() noexcept
		{
			return format_type_name<Src, J, I, Last, N>({});
		}
		template<basic_static_string Name>
		consteval auto format_type_name() noexcept
		{
#if defined(__clang__) || defined(__GNUC__)
			constexpr auto offset_start = Name.find_first('=') + 2;
			constexpr auto offset_end = Name.find_last(']');
			constexpr auto trimmed_length = offset_end - offset_start + 1;

			return format_type_name<Name, 0, offset_start, offset_end, trimmed_length>();
#elif defined(_MSC_VER)
			constexpr auto offset_start = Name.find_first('<') + 1;
			constexpr auto offset_end = Name.find_last('>');
			constexpr auto trimmed_length = offset_end - offset_start + 1;

			return format_type_name<Name, 0, offset_start, offset_end, trimmed_length>();
#else
			return Name;
#endif
		}

		template<typename T>
		[[nodiscard]] constexpr std::basic_string_view<char> generate_type_name() noexcept
		{
			constexpr auto &value = auto_constant<format_type_name<SEK_PRETTY_FUNC>()>::value;
			return std::basic_string_view<char>{value.begin(), value.end()};
		}
	}	 // namespace detail

	/** @brief Helper type used to generate a type name string for the specified type.
	 * @warning Consistency of auto-generated type names across different compilers is not guaranteed.
	 * To generate consistent type names, overload `type_name` for the desired type. */
	template<typename T>
	struct type_name
	{
		constexpr static std::string_view value = detail::generate_type_name<T>();
	};
	/** @brief Alias for `type_name<T>::value`. */
	template<typename T>
	constexpr inline std::string_view type_name_v = type_name<T>::value;

	/* Type name overloads for "default" string types. */
	template<>
	struct type_name<std::basic_string<char>>
	{
		constexpr static std::string_view value = "std::string";
	};
	template<>
	struct type_name<std::basic_string<wchar_t>>
	{
		constexpr static std::string_view value = "std::wstring";
	};
	template<>
	struct type_name<std::basic_string<char8_t>>
	{
		constexpr static std::string_view value = "std::u8string";
	};
	template<>
	struct type_name<std::basic_string<char16_t>>
	{
		constexpr static std::string_view value = "std::u16string";
	};
	template<>
	struct type_name<std::basic_string<char32_t>>
	{
		constexpr static std::string_view value = "std::u32string";
	};
	template<>
	struct type_name<std::basic_string_view<char>>
	{
		constexpr static std::string_view value = "std::string_view";
	};
	template<>
	struct type_name<std::basic_string_view<wchar_t>>
	{
		constexpr static std::string_view value = "std::wstring_view";
	};
	template<>
	struct type_name<std::basic_string_view<char8_t>>
	{
		constexpr static std::string_view value = "std::u8string_view";
	};
	template<>
	struct type_name<std::basic_string_view<char16_t>>
	{
		constexpr static std::string_view value = "std::u16string_view";
	};
	template<>
	struct type_name<std::basic_string_view<char32_t>>
	{
		constexpr static std::string_view value = "std::u32string_view";
	};
}	 // namespace sek