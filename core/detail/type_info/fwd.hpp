/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "../../define.h"
#include <initializer_list>

namespace sek
{
	namespace detail
	{
		struct type_handle_t;
		struct type_data;
	}	 // namespace detail

	class type_error;
	enum class type_errc;

	template<typename>
	class type_factory;

	class type_info;
	class type_query;
	class type_database;

	class any;

	template<typename T>
	[[nodiscard]] any forward_any(T &&);
	template<typename T, typename... Args>
	[[nodiscard]] any make_any(Args &&...);
	template<typename T, typename U, typename... Args>
	[[nodiscard]] any make_any(std::initializer_list<U>, Args &&...);
}	 // namespace sek