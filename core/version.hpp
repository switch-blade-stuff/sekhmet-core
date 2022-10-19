/*
 * Created by switchblade on 10/08/22
 */

#pragma once

#include <compare>

#include "hash.hpp"
#include <fmt/format.h>

namespace sek
{
	/** @brief Structure holding 3 integers representing a "semantic" version number or a compatible version format. */
	struct version
	{
		template<typename, typename, typename>
		friend struct fmt::formatter;

	private:
		template<typename U, typename C>
		[[nodiscard]] constexpr static U parse_char(C c)
		{
			return c >= '0' && c <= '9' ? static_cast<U>(c - '0') : throw std::runtime_error("Invalid version string");
		}

		template<std::size_t I = 0, std::forward_iterator Iter, typename U, typename... Us>
		constexpr static void parse_string(Iter first, Iter last, U &cmp, Us &...cmps) noexcept
		{
			for (std::iter_value_t<Iter> c; first != last && (c = *first++) != '\0' && c != '.';)
			{
				/* Keep parsing this component until a separator. */
				const U a = parse_char<U>(c);
				const U b = cmp * 10;
				cmp = a + b;
			}
			if constexpr (sizeof...(Us) != 0) parse_string<I + 1>(first, last, cmps...);
		}
		template<typename C, typename Iter, typename T>
		constexpr static Iter write_string(Iter out, T val)
		{
			if constexpr (std::is_signed_v<T>)
				if (val < 0)
				{
					*out++ = '-';
					val = -val;
				}

			T div = 1;
			T digits = 1;
			for (; div <= val / 10; ++digits) div *= 10;
			for (; digits > 0; --digits)
			{
				*out++ = alphabet<C>[val / div];
				val %= div;
				div /= 10;
			}
			return out;
		}

	public:
		constexpr version() noexcept = default;
		/** Initializes a version from the major, minor & patch components.
		 * @param major Major component.
		 * @param minor Minor component.
		 * @param patch Patch component. */
		constexpr version(std::uint16_t major, std::uint16_t minor, std::uint32_t patch) noexcept
			: major(major), minor(minor), patch(patch)
		{
		}

		/** Initializes a version from a from a character range.
		 * @note Version string must contain base-10 integers separated with dots ('.'). */
		template<std::forward_iterator Iter>
		constexpr version(Iter first, Iter last)
		{
			parse_string(first, last, major, minor, patch);
		}
		/** @copydoc uuid */
		template<std::ranges::forward_range R>
		constexpr explicit version(const R &str) : version(std::ranges::begin(str), std::ranges::end(str))
		{
		}
		/**  Initializes a version from a from a string of an explicit length. */
		template<typename C>
		constexpr version(const C *str, std::size_t n) noexcept : version(str, str + n)
		{
		}
		/**  Initializes a version from a from a string. */
		template<typename C>
		constexpr version(const C *str) noexcept : version(std::basic_string_view{str})
		{
		}

		/** Returns 64-bit integer representation of the version. */
		[[nodiscard]] constexpr std::uint64_t as_uint64() const noexcept
		{
			return (static_cast<std::uint64_t>(major) << 48) | (static_cast<std::uint64_t>(minor) << 32) |
				   (static_cast<std::uint64_t>(patch));
		}

		constexpr void swap(version &other) noexcept
		{
			std::swap(major, other.major);
			std::swap(minor, other.minor);
			std::swap(patch, other.patch);
		}
		friend constexpr void swap(version &a, version &b) noexcept { a.swap(b); }

		[[nodiscard]] constexpr auto operator<=>(const version &other) const noexcept
		{
			return as_uint64() <=> other.as_uint64();
		}
		[[nodiscard]] constexpr bool operator==(const version &other) const noexcept
		{
			return as_uint64() == other.as_uint64();
		}

		std::uint16_t major = 0;
		std::uint16_t minor = 0;
		std::uint32_t patch = 0;

	private:
		template<typename C>
		constexpr static C alphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	};

	template<std::size_t I>
	[[nodiscard]] constexpr auto &get(version &v) noexcept
	{
		if constexpr (I == 0)
			return v.major;
		else if constexpr (I == 1)
			return v.minor;
		else
			return v.patch;
	}
	template<std::size_t I>
	[[nodiscard]] constexpr auto &get(const version &v) noexcept
	{
		if constexpr (I == 0)
			return v.major;
		else if constexpr (I == 1)
			return v.minor;
		else
			return v.patch;
	}

	[[nodiscard]] constexpr hash_t hash(const version &v) noexcept { return hash(v.as_uint64()); }

	namespace literals
	{
		[[nodiscard]] constexpr version operator""_ver(const char *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const char8_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const char16_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const char32_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
		[[nodiscard]] constexpr version operator""_ver(const wchar_t *str, std::size_t n) noexcept
		{
			return version{str, str + n};
		}
	}	 // namespace literals
}	 // namespace sek

template<>
struct std::hash<sek::version>
{
	[[nodiscard]] constexpr sek::hash_t operator()(const sek::version &v) const noexcept { return sek::hash(v); }
};
template<>
struct std::tuple_size<sek::version> : std::integral_constant<std::size_t, 3>
{
};

template<typename C>
struct fmt::formatter<sek::version, C>
{
	auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
	{
		const auto pos = ctx.begin();
		if (pos != ctx.end() && *pos != '}') [[unlikely]]
			throw format_error("Invalid format");
		return pos;
	}
	template<typename Ctx>
	auto format(const sek::version &v, Ctx &ctx) const -> decltype(ctx.out())
	{
		auto out = ctx.out();
		out = sek::version::write_string<C>(out, v.major);
		*out++ = '.';
		out = sek::version::write_string<C>(out, v.minor);
		*out++ = '.';
		out = sek::version::write_string<C>(out, v.patch);
		return out;
	}
};