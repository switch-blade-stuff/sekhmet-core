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

	auto type_info::find_overload(auto &range, std::span<type_info> args)
	{
		auto overload = range.begin(), last = range.end();
		for (; overload != last; ++overload)
		{
			constexpr auto pred = [](detail::func_arg_data a, type_info b) { return type_info{a.type} == b; };
			if (std::ranges::equal(overload->args, args, pred)) return overload;
		}
		return last;
	}
	auto type_info::find_overload(auto &range, std::span<any> args)
	{
		auto overload = range.begin(), last = range.end();
		for (; overload != last; ++overload)
		{
			constexpr auto pred = [](detail::func_arg_data a, const any &b)
			{
				/* Both type and const-ness must match. */
				return a.is_const == b.is_const() && type_info{a.type} == b.type();
			};
			if (std::ranges::equal(overload->args, args, pred)) return overload;
		}
		return last;
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
		if (valid()) [[likely]]
		{
			const auto iter = m_data->constants.find(name);
			return iter != m_data->constants.end() && type_info{iter->type} == type;
		}
		return false;
	}
	bool type_info::inherits(type_info type) const noexcept
	{
		if (valid()) [[likely]]
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

	any type_info::attribute(type_info type) const
	{
		if (valid()) [[likely]]
		{
			const auto iter = m_data->attributes.find(type.m_data);
			return iter != m_data->attributes.end() ? iter->get() : any{};
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
	any type_info::construct(std::span<any> args) const
	{
		if (valid()) [[likely]]
		{
			const auto iter = find_overload(m_data->constructors, args);
			return iter != m_data->constructors.end() ? iter->invoke(args) : any{};
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
