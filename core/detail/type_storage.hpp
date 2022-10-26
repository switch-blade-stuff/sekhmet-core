/*
 * Created by switchblade on 2021-12-03
 */

#pragma once

#include <bit>
#include <cstddef>

namespace sek
{
	// clang-format off
	template<typename T, std::size_t Size = 1> requires (Size >= 1)
	class type_storage
	{
	public:
		constexpr type_storage() noexcept : m_bytes() {}
		constexpr type_storage(const type_storage &other) noexcept : m_bytes(other.m_bytes) {}
		constexpr type_storage &operator=(const type_storage &other) noexcept
		{
			m_bytes = other.m_bytes;
			return *this;
		}
		constexpr type_storage(type_storage &&other) noexcept : m_bytes(std::move(other.m_bytes)) {}
		constexpr type_storage &operator=(type_storage &&other) noexcept
		{
			m_bytes = std::move(other.m_bytes);
			return *this;
		}

		[[nodiscard]] constexpr T *get() noexcept { return m_data; }
		[[nodiscard]] constexpr const T *get() const noexcept { return m_data; }
		[[nodiscard]] constexpr void *data() noexcept { return static_cast<void *>(m_data); }
		[[nodiscard]] constexpr const void *data() const noexcept { return static_cast<const void *>(m_data); }

	private:
		union
		{
			std::byte m_bytes[Size * sizeof(T)];
			T m_data[Size];
		};
	};
	// clang-format off
}	 // namespace sek