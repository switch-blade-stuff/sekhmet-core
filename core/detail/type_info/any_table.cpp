/*
 * Created by switchblade on 2022-10-03.
 */

#include "any_table.hpp"

#include "type_db.hpp"
#include <fmt/format.h>

namespace sek
{
	// clang-format off
	const detail::table_type_data *any_table::assert_data(const detail::type_data *data)
	{
		if (data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::UNEXPECTED_EMPTY_ANY));
		else if (data->table_data == nullptr) [[unlikely]]
			throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("<{}> is not a table-like range", data->name));
		return data->table_data;
	}
	// clang-format on
}	 // namespace sek