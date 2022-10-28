/*
 * Created by switchblade on 2022-10-03.
 */

#include "any_tuple.hpp"

#include <fmt/format.h>

namespace sek
{
	// clang-format off
	const detail::tuple_type_data *any_tuple::assert_data(const detail::type_data *data)
	{
		if (data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::UNEXPECTED_EMPTY_ANY));
		else if (data->tuple_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("<{}> is not a tuple-like type", data->name));
		return data->tuple_data;
	}
	// clang-format on
}	 // namespace sek