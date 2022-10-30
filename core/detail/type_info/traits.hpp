/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "../../meta.hpp"

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
}	 // namespace sek::detail