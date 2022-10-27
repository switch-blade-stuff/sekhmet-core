/*
 * Created by switchblade on 2022-10-04.
 */

#include "type_info.hpp"

#include <fmt/format.h>

SEK_EXPORT_TYPE_INFO(sek::any)
SEK_EXPORT_TYPE_INFO(sek::type_info)

namespace sek
{
	[[nodiscard]] static std::string format_args(std::span<type_info> args)
	{
		std::string result;
		for (auto type : args)
		{
			if (!result.empty()) [[likely]]
				result.append(", ");
			result.append(type.name());
		}
		return result;
	}
	[[nodiscard]] static std::string format_args(std::span<any> args)
	{
		std::string result;
		for (auto &arg : args)
		{
			if (!result.empty()) [[likely]]
				result.append(", ");
			result.append(arg.type().name());
		}
		return result;
	}

	bool type_info::inherits(type_info type) const noexcept
	{
		for (auto &parent : m_data->parents)
		{
			const auto parent_type = type_info{parent.type};
			if (parent_type == type || parent_type.inherits(type)) [[likely]]
				return true;
		}
		return false;
	}

	expected<any, std::error_code> type_info::construct(std::nothrow_t, std::span<any> args)
	{
		for (auto &ctor : m_data->constructors)
		{
			constexpr auto pred = [](detail::func_arg_data a, const any &b)
			{
				/* Both type and const-ness must match. */
				return a.is_const == b.is_const() && type_info{a.type} == b.type();
			};
			if (std::ranges::equal(ctor.args, args, pred)) return ctor.invoke(args);
		}
		return unexpected{make_error_code(type_errc::INVALID_CONSTRUCTOR)};
	}
	any type_info::construct(std::span<any> args)
	{
		for (auto &ctor : m_data->constructors)
		{
			constexpr auto pred = [](detail::func_arg_data a, const any &b)
			{
				/* Both type and const-ness must match. */
				return a.is_const == b.is_const() && type_info{a.type} == b.type();
			};
			if (std::ranges::equal(ctor.args, args, pred)) return ctor.invoke(args);
		}

		// clang-format off
		throw type_error(make_error_code(type_errc::INVALID_CONSTRUCTOR),
						 fmt::format("No constructor with overload ({}) found for type <{}>",
									 format_args(args), name()));
		// clang-format on
	}
}	 // namespace sek
