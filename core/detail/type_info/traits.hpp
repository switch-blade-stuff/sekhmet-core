/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "fwd.hpp"
#include <type_traits>

namespace sek::detail
{
	template<typename>
	struct callable_traits;

	template<typename R, typename C, typename... Args>
	struct callable_traits<R (C::*)(Args...)>
	{
		using return_type = R;
		using arg_types = type_seq_t<Args...>;

		const static bool is_const = false;
	};
	template<typename R, typename C, typename... Args>
	struct callable_traits<R (C::*)(Args...) const>
	{
		using return_type = R;
		using arg_types = type_seq_t<Args...>;

		const static bool is_const = true;
	};
	template<typename R, typename C, typename I, typename... Args>
	struct callable_traits<R (C::*)(I *, Args...)>
	{
		using return_type = R;
		using instance_type = I;
		using arg_types = type_seq_t<Args...>;

		const static bool is_const = false;
	};
	template<typename R, typename C, typename I, typename... Args>
	struct callable_traits<R (C::*)(I *, Args...) const>
	{
		using return_type = R;
		using instance_type = I;
		using arg_types = type_seq_t<Args...>;

		const static bool is_const = true;
	};

	template<typename R, typename... Args, R (*F)(Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (*F)(I *, Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (*F)(const I *, Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = const I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (*F)(volatile I *, Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = volatile I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (*F)(const volatile I *, Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = const volatile I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...)>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = const I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) volatile>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = volatile I;
		using arg_types = type_seq_t<Args...>;
	};
	template<typename R, typename I, typename... Args, R (I::*F)(Args...) const volatile>
	struct func_traits<F>
	{
		using return_type = R;
		using instance_type = const volatile I;
		using arg_types = type_seq_t<Args...>;
	};

	// clang-format off
	template<typename T, typename... Ts>
	concept allowed_types = std::disjunction_v<std::is_same<Ts, std::remove_cvref_t<T>>...>;
	template<typename T>
	concept string_like_type = std::ranges::contiguous_range<T> && std::constructible_from<
		std::basic_string_view<std::ranges::range_value_t<T>>, std::ranges::iterator_t<T>, std::ranges::iterator_t<T>>;
	// clang-format on
}	 // namespace sek::detail