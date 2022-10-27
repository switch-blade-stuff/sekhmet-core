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
		template<auto>
		struct func_traits;

		struct type_data;
		struct type_handle;
		struct tuple_type_data;
		struct range_type_data;
		struct table_type_data;
	}	 // namespace detail

	class type_error;
	enum class type_errc;

	class type_info;
	class type_attribute_info;
	class type_parent_info;
	class type_constant_info;
	class type_conversion_info;
	class type_constructor_info;
	class type_function_info;
	class type_property_info;

	class type_query;
	class type_database;

	template<typename>
	struct type_info_factory;

	class any;
	class any_tuple;
	class any_range;
	class any_table;
	class any_string;

	template<typename T>
	[[nodiscard]] any forward_any(T &&);
	template<typename T, typename... Args>
	[[nodiscard]] any make_any(Args &&...);
	template<typename T, typename U, typename... Args>
	[[nodiscard]] any make_any(std::initializer_list<U>, Args &&...);
}	 // namespace sek