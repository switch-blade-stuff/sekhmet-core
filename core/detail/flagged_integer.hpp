/*
 * Created by switchblade on 2021-11-26
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <limits>

namespace sek
{
	/** @brief Helper structure used to store an integer and a flag in one of it's bits. */
	template<std::integral I>
	class flagged_integer_t
	{
		constexpr static I mask = std::numeric_limits<I>::max() << 1;

	public:
		constexpr flagged_integer_t() noexcept = default;
		constexpr explicit flagged_integer_t(I v, bool f = false) noexcept { m_data = (v * 2) | (f & 1); }

		[[nodiscard]] constexpr I value() const noexcept { return m_data / 2; }
		constexpr I value(I value) noexcept
		{
			m_data = (value * 2) | (m_data & 1);
			return value;
		}

		[[nodiscard]] constexpr bool flag() const noexcept { return m_data & 1; }
		constexpr bool flag(bool value) noexcept
		{
			m_data = (m_data & mask) | value;
			return value;
		}
		constexpr void toggle() noexcept { m_data ^= 1; }

		[[nodiscard]] constexpr bool operator==(const flagged_integer_t &) const noexcept = default;

	private:
		I m_data = 0;
	};

	template<std::integral I>
	flagged_integer_t(I) -> flagged_integer_t<I>;
	template<std::integral I>
	flagged_integer_t(I, bool) -> flagged_integer_t<I>;
}	 // namespace sek