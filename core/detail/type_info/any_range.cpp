/*
 * Created by switchblade on 2022-10-03.
 */

#include "any_range.hpp"

#include "type_db.hpp"
#include <fmt/format.h>

namespace sek
{
	const detail::range_type_data *any_range::assert_data(const detail::type_data *data)
	{
		if (data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::UNEXPECTED_EMPTY_ANY));
		else if (data->range_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("<{}> is not a range", data->name));
		return data->range_data;
	}
}	 // namespace sek