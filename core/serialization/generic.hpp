/*
 * Created by switchblade on 10/22/22.
 */

#pragma once

#include "../type_info.hpp"
#include "detail/archive_error.hpp"
#include <fmt/format.h>

namespace sek
{
	// clang-format off
	template<typename T, typename A>
	void serialize(const T &value, A &archive) requires std::is_enum_v<T>
	{
		const auto type = type_info::get<T>();
		const auto constants = type.constants();
		for (auto pos = constants.begin(); pos != constants.end(); ++pos)
			if (const auto cast = pos.get().cast(type); !cast.empty() && *cast.template get<T>() == value)
			{
				archive.set(pos->name());
				return;
			}

		archive.set(static_cast<std::underlying_type_t<T>>(value));
	}
	template<typename T, typename A, typename... Args>
	void deserialize(T &value, A &archive) requires std::is_enum_v<T>
	{
		if (const auto name = archive.template try_get<std::string_view>(); !name)
			value = archive.template get<std::underlying_type_t<T>>();
		else
		{
			const auto type = type_info::get<T>();
			const auto underlying = type_info::get<std::underlying_type_t<T>>();
			if (const auto constant = type.constant(name); !constant.empty()) [[likely]]
			{
				if (auto cast = constant.get().cast(type); !cast.empty()) [[likely]]
				{
					value = *cast.template get<T>();
					return;
				}
				else if (cast = constant.get().cast(underlying); !cast.empty()) [[likely]]
				{
					value = *cast.template get<std::underlying_type_t<T>>();
					return;
				}
			}
			throw archive_error(make_error_code(archive_errc::INVALID_DATA), fmt::format("Invalid enum value \"{}\"", name));
		}
	}
	// clang-format on
}	 // namespace sek