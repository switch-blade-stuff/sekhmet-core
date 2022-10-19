/*
 * Created by switchblade on 2021-10-25
 */

#pragma once

#include <concepts>
#include <memory>

#include "../assert.hpp"

namespace sek::detail
{
	template<typename Alloc, typename T>
	using rebind_alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

	template<typename Alloc>
	[[nodiscard]] constexpr bool alloc_eq(const Alloc &a, const Alloc &b) noexcept
	{
		// clang-format off
		if constexpr (std::allocator_traits<Alloc>::is_always_equal::value)
			return true;
		else if constexpr (requires { { a == b }; })
			return a == b;
		else
			return false;
		// clang-format on
	}

	template<typename Alloc>
	[[nodiscard]] constexpr Alloc alloc_copy(const Alloc &alloc)
	{
		return std::allocator_traits<Alloc>::select_on_container_copy_construction(alloc);
	}

	template<typename Alloc>
	constexpr void alloc_move_assign(Alloc &lhs, Alloc &rhs) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value) lhs = std::move(rhs);
	}
	template<typename Alloc>
	constexpr void alloc_copy_assign(Alloc &lhs, const Alloc &rhs) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::propagate_on_container_copy_assignment::value) lhs = rhs;
	}

	template<typename Alloc>
	constexpr void alloc_assert_swap([[maybe_unused]] const Alloc &lhs, [[maybe_unused]] const Alloc &rhs) noexcept
	{
		SEK_ASSERT(std::allocator_traits<Alloc>::propagate_on_container_swap::value || alloc_eq(lhs, rhs));
	}
	template<typename Alloc>
	constexpr void alloc_swap(Alloc &lhs, Alloc &rhs) noexcept
	{
		if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value)
		{
			using std::swap;
			swap(lhs, rhs);
		}
	}

	template<typename... As>
	constexpr bool nothrow_alloc_construct = std::conjunction_v<std::is_nothrow_default_constructible<As>...>;

	// clang-format off
	template<typename... As>
	constexpr bool nothrow_alloc_copy = std::conjunction_v<std::bool_constant<std::allocator_traits<As>::is_always_equal::value &&
																			  std::is_nothrow_copy_constructible_v<As>>...>;
	template<typename A0, typename A1>
	constexpr bool nothrow_alloc_transfer = nothrow_alloc_copy<A0> && std::is_nothrow_move_constructible_v<A1>;
	// clang-format on

	template<typename... As>
	constexpr bool nothrow_alloc_move_construct = std::conjunction_v<std::is_nothrow_move_constructible<As>...>;
	// clang-format off
	template<typename... As>
	constexpr bool nothrow_alloc_move_assign = std::conjunction_v<std::bool_constant<
			(std::allocator_traits<As>::propagate_on_container_move_assignment::value && std::is_nothrow_move_assignable_v<As>) ||
			std::allocator_traits<As>::is_always_equal::value>...>;
	// clang-format on
}	 // namespace sek::detail