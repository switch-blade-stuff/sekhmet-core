/*
 * Created by switchblade on 2022-10-04.
 */

#include "type_db.hpp"
#include <fmt/format.h>

SEK_EXPORT_TYPE_INFO(sek::any)
SEK_EXPORT_TYPE_INFO(sek::type_info)

namespace sek
{
	[[maybe_unused]] [[nodiscard]] static std::string format_args(std::span<type_info> args)
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
	[[maybe_unused]] [[nodiscard]] static std::string format_args(std::span<any> args)
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

	bool type_info::check_arg(const detail::func_arg_data &exp, any &value) noexcept
	{
		const auto a = type_info{exp.type};
		const auto b = value.type();
		return exp.is_const >= value.is_const() && (a == b || b.inherits(a) || b.convertible_to(a));
	}
	auto type_info::find_overload(std::vector<detail::ctor_data> &range, std::span<any> args) noexcept
	{
		for (auto overload = range.begin(); overload != range.end(); ++overload)
			if (std::ranges::equal(overload->args, args, check_arg)) return overload;
		return range.end();
	}
	auto type_info::find_overload(std::vector<detail::func_overload> &range, any &instance, std::span<any> args) noexcept
	{
		for (auto overload = range.begin(); overload != range.end(); ++overload)
		{
			const auto instance_type = type_info{overload->instance_type};
			if (instance_type == instance.type() && overload->is_const == instance.is_const() &&
				std::ranges::equal(overload->args, args, check_arg))
				return overload;
		}
		return range.end();
	}
	std::error_code type_info::convert_args(std::span<const detail::func_arg_data> expected, std::span<any> args)
	{
		SEK_ASSERT_ALWAYS(args.size() <= std::numeric_limits<std::uint16_t>::max());

		for (std::uint16_t i = 0; i < args.size(); ++i)
		{
			const auto expected_type = type_info{expected[i].type};
			const auto actual_type = args[i].type();

			type_errc error = {};
			if (expected[i].is_const <= args[i].is_const()) [[likely]]
			{
				if (expected_type == actual_type) [[likely]]
					continue;

				if (actual_type.inherits(expected_type))
				{
					if (args[i].is_ref()) [[likely]]
					{
						args[i] = args[i].as(expected_type);
						continue;
					}
					error = type_errc::EXPECTED_REF_ANY;
				}

				if (actual_type.convertible_to(expected_type))
				{
					args[i] = args[i].conv(expected_type);
					continue;
				}
			}
			return make_error_code(error | type_errc::INVALID_PARAM | static_cast<std::uint16_t>(i));
		}
		return {};
	}

	bool type_info::inherits(type_info type) const noexcept
	{
		if (valid() && type.valid()) [[likely]]
		{
			auto &parents = m_data->parents;
			if (parents.contains(type.m_data)) [[likely]]
				return true;
			for (auto &parent : parents)
			{
				const auto parent_type = type_info{parent.type};
				if (parent_type.inherits(type)) [[likely]]
					return true;
			}
		}
		return false;
	}
	bool type_info::convertible_to(type_info type) const noexcept
	{
		if (valid() && type.valid()) [[likely]]
		{
			auto &conversions = m_data->conversions;
			if (conversions.contains(type.m_data)) [[likely]]
				return true;
			for (auto &conv : conversions)
			{
				const auto conv_type = type_info{conv.to_type};
				if (conv_type.convertible_to(type)) [[likely]]
					return true;
			}
		}
		return false;
	}

	bool type_info::has_attribute(type_info type) const noexcept
	{
		return valid() && m_data->attributes.contains(type.m_data);
	}
	bool type_info::has_constant(std::string_view name) const noexcept
	{
		return valid() && m_data->constants.contains(name);
	}
	bool type_info::has_constant(std::string_view name, type_info type) const noexcept
	{
		if (valid() && type.valid()) [[likely]]
		{
			const auto iter = m_data->constants.find(name);
			return iter != m_data->constants.end() && type_info{iter->type} == type;
		}
		return false;
	}

	any type_info::attribute(type_info type) const
	{
		if (valid()) [[likely]]
		{
			const auto iter = m_data->attributes.find(type.m_data);
			return iter != m_data->attributes.end() ? iter->get() : any{};
		}
		return {};
	}
	any type_info::construct(std::span<any> args) const
	{
		if (valid()) [[likely]]
		{
			const auto iter = find_overload(m_data->constructors, args);
			if (iter != m_data->constructors.end()) [[likely]]
			{
				/* No need to check the error code, since the overload is already compatible. */
				convert_args(iter->args, args);
				return iter->invoke(args);
			}
		}
		return {};
	}
	any type_info::constant(std::string_view name) const
	{
		if (valid()) [[likely]]
		{
			const auto iter = m_data->constants.find(name);
			return iter != m_data->constants.end() ? iter->get() : any{};
		}
		return {};
	}

	bool constant_info::has_attribute(type_info type) const noexcept
	{
		return base_t::attributes.contains(type.m_data);
	}
	any constant_info::attribute(type_info type) const
	{
		const auto iter = base_t::attributes.find(type.m_data);
		return iter != base_t::attributes.end() ? iter->get() : any{};
	}
}	 // namespace sek
