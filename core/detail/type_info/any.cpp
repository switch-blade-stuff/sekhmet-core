/*
 * Created by switchblade on 2022-10-03.
 */

#include "any.hpp"

#include "type_db.hpp"
#include <fmt/format.h>

namespace sek
{
	void any::throw_bad_cast(type_info from, type_info to)
	{
		// clang-format off
		throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("Invalid parent cast - <{}> is not a base of <{}>",
																			   from.name(), to.name()));
		// clang-format on
	}
	void any::throw_bad_conv(type_info from, type_info to)
	{
		// clang-format off
		throw type_error(make_error_code(type_errc::INVALID_TYPE), fmt::format("Cannot convert from <{}> to <{}> - no conversion defined",
																			   from.name(), to.name()));
		// clang-format on
	}

	void any::destroy()
	{
		if (!empty() && !is_ref()) [[likely]]
		{
			m_type->dtor.invoke(data());
			m_storage.do_delete();
		}
	}
	void any::copy_init(const any &other)
	{
		if (!other.empty()) [[likely]]
		{
			/* If the `copy_init` functor is set to `nullptr`, there is a custom non-default copy constructor
			 * specified by the user. In that case, use that constructor. */
			if (!other.m_type->any_funcs.copy_init)
				move_init(type().construct(std::in_place, other.ref()));
			else
			{
				other.m_type->any_funcs.copy_init(other, *this);
				m_type = other.m_type;
			}
		}
	}
	void any::copy_assign(const any &other)
	{
		if (!empty() && type() == other.type())
		{
			/* If the `copy_assign` functor is set to `nullptr`, there is a custom non-default copy constructor
			 * specified by the user. In that case, use that constructor in conjunction with a reset. */
			if (!other.empty() && other.m_type->any_funcs.copy_assign)
			{
				other.m_type->any_funcs.copy_assign(other, *this);
				m_type = other.m_type;
				return;
			}
		}

		reset();
		copy_init(other);
	}

	any any::as(type_info to_type) { return as_impl(to_type, is_const()); }
	any any::as(type_info to_type) const { return as_impl(to_type, true); }
	any any::as_impl(type_info to_type, bool const_ref) const
	{
		/* Do not use `ref()` to enable const-ness override. */
		if (type() == to_type) return any{m_type, {cdata(), const_ref}};

		const auto to_any = [&](detail::type_handle h, const void *ptr) { return any{h.get(), {ptr, const_ref}}; };
		const auto data_ptr = cdata();

		if (auto parent = m_type->parents.find(to_type.m_data); parent != m_type->parents.end()) [[likely]]
			return to_any(parent->type, parent->cast(data_ptr));
		for (auto &parent : m_type->parents)
		{
			auto cast = to_any(parent.type, parent.cast(data_ptr)).as(to_type);
			if (!cast.empty()) [[likely]]
				return cast;
		}
		return {};
	}

	expected<any_range, std::error_code> any::range(std::nothrow_t)
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->range_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_range{std::in_place, ref()};
	}
	expected<any_range, std::error_code> any::range(std::nothrow_t) const
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->range_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_range{std::in_place, ref()};
	}
	expected<any_table, std::error_code> any::table(std::nothrow_t)
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->table_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_table{std::in_place, ref()};
	}
	expected<any_table, std::error_code> any::table(std::nothrow_t) const
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->table_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_table{std::in_place, ref()};
	}
	expected<any_tuple, std::error_code> any::tuple(std::nothrow_t)
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->tuple_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_tuple{std::in_place, ref()};
	}
	expected<any_tuple, std::error_code> any::tuple(std::nothrow_t) const
	{
		if (empty()) [[unlikely]]
			return unexpected{make_error_code(type_errc::UNEXPECTED_EMPTY_ANY)};
		else if (m_type->tuple_data == nullptr) [[unlikely]]
			return unexpected{make_error_code(type_errc::INVALID_TYPE)};
		return any_tuple{std::in_place, ref()};
	}

	any_range any::range() { return any_range{ref()}; }
	any_range any::range() const { return any_range{ref()}; }
	any_table any::table() { return any_table{ref()}; }
	any_table any::table() const { return any_table{ref()}; }
	any_tuple any::tuple() { return any_tuple{ref()}; }
	any_tuple any::tuple() const { return any_tuple{ref()}; }

	bool operator==(const any &a, const any &b) noexcept
	{
		if (a.type().valid() == b.type().valid() && a.m_type->any_funcs.cmp_eq != nullptr) [[likely]]
			return a.m_type->any_funcs.cmp_eq(a.data(), b.data());
		return false;
	}
	bool operator<(const any &a, const any &b) noexcept
	{
		if (a.type().valid() == b.type().valid() && a.m_type->any_funcs.cmp_lt != nullptr) [[likely]]
			return a.m_type->any_funcs.cmp_lt(a.data(), b.data());
		return false;
	}
	bool operator<=(const any &a, const any &b) noexcept
	{
		if (a.type().valid() == b.type().valid() && a.m_type->any_funcs.cmp_le != nullptr) [[likely]]
			return a.m_type->any_funcs.cmp_le(a.data(), b.data());
		return false;
	}
	bool operator>(const any &a, const any &b) noexcept
	{
		if (a.type().valid() == b.type().valid() && a.m_type->any_funcs.cmp_gt != nullptr) [[likely]]
			return a.m_type->any_funcs.cmp_gt(a.data(), b.data());
		return false;
	}
	bool operator>=(const any &a, const any &b) noexcept
	{
		if (a.type().valid() == b.type().valid() && a.m_type->any_funcs.cmp_ge != nullptr) [[likely]]
			return a.m_type->any_funcs.cmp_ge(a.data(), b.data());
		return false;
	}
}	 // namespace sek