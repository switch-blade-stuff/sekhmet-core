/*
 * Created by switchblade on 05/10/22
 */

#include "type_db.hpp"

#include <fmt/format.h>

namespace sek
{
	shared_guard<type_database *> type_database::instance()
	{
		static type_database db;
		return {&db, &db.m_mtx};
	}

	detail::type_data *type_database::reflect_impl(detail::type_handle handle)
	{
		auto iter = m_type_table.find(handle.name);
		if (iter == m_type_table.end()) [[likely]]
		{
			const auto type = type_info{handle};
			iter = m_type_table.insert(type).first;

			/* Add the type to the attribute map. */
			for (auto &attr : type.attributes())
			{
				const auto attr_iter = m_attr_table.try_emplace(attr.type().name()).first;
				attr_iter->second.try_insert(type);
			}
		}
		return iter->m_data;
	}
	type_info type_database::get(std::string_view name)
	{
		const auto iter = m_type_table.find(name);
		if (iter != m_type_table.end()) [[likely]]
			return *iter;
		return {};
	}
	void type_database::reset(std::string_view type)
	{
		if (const auto iter = m_type_table.find(type); iter != m_type_table.end()) [[likely]]
		{
			/* Remove the type from the attribute map. */
			for (auto &attr : iter->attributes())
			{
				const auto attr_iter = m_attr_table.find(attr.type().name());
				if (attr_iter != m_attr_table.end()) [[likely]]
					attr_iter->second.erase(type);
			}

			/* Reset the type to its original "unreflected" state and remove from the set. */
			iter->m_data->reset();
			m_type_table.erase(iter);
		}
	}

	type_query &type_query::with_attribute(type_info type)
	{
		if (const auto iter = m_db.m_attr_table.find(type.name()); iter != m_db.m_attr_table.end()) [[likely]]
		{
			if (!m_started) [[unlikely]]
			{
				/* If the query does not have a set yet, copy the attribute's set. */
				m_types = iter->second;
				m_started = true;
			}
			else
			{
				/* Otherwise, remove all types that are not part of the attribute's set. */
				for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
					if (!iter->second.contains(*pos)) m_types.erase(pos);
			}
		}
		return *this;
	}
	type_query &type_query::with_constant(std::string_view name)
	{
		if (!m_started) [[unlikely]]
		{
			/* If the query does not have a set yet, go through each reflected type & check it. */
			for (auto &candidate : m_db.m_type_table)
			{
				if (candidate.has_constant(name)) [[unlikely]]
					m_types.insert(candidate);
			}
			m_started = true;
		}
		else
		{
			/* Otherwise, remove all types that are not part of the attribute's set. */
			for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
			{
				if (!pos->has_constant(name)) [[likely]]
					m_types.erase(pos);
			}
		}
		return *this;
	}
	type_query &type_query::with_constant(std::string_view name, type_info type)
	{
		if (!m_started) [[unlikely]]
		{
			/* If the query does not have a set yet, go through each reflected type & check it. */
			for (auto &candidate : m_db.m_type_table)
			{
				if (candidate.has_constant(name, type)) [[unlikely]]
					m_types.insert(candidate);
			}
			m_started = true;
		}
		else
		{
			/* Otherwise, remove all types that are not part of the attribute's set. */
			for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
			{
				if (!pos->has_constant(name, type)) [[likely]]
					m_types.erase(pos);
			}
		}
		return *this;
	}

	type_query &type_query::inherits_from(type_info type)
	{
		if (!m_started) [[unlikely]]
		{
			/* If the query does not have a set yet, go through each reflected type & check it. */
			for (auto &candidate : m_db.m_type_table)
			{
				if (candidate.inherits(type)) [[unlikely]]
					m_types.insert(candidate);
			}
			m_started = true;
		}
		else
		{
			/* Otherwise, remove all types that are not part of the attribute's set. */
			for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
			{
				if (!pos->inherits(type)) [[likely]]
					m_types.erase(pos);
			}
		}
		return *this;
	}
	type_query &type_query::convertible_to(type_info type)
	{
		if (!m_started) [[unlikely]]
		{
			/* If the query does not have a set yet, go through each reflected type & check it. */
			for (auto &candidate : m_db.m_type_table)
			{
				if (candidate.convertible_to(type)) [[unlikely]]
					m_types.insert(candidate);
			}
			m_started = true;
		}
		else
		{
			/* Otherwise, remove all types that are not part of the attribute's set. */
			for (auto pos = m_types.end(), end = m_types.begin(); pos-- != end;)
			{
				if (!pos->convertible_to(type)) [[likely]]
					m_types.erase(pos);
			}
		}
		return *this;
	}
}	 // namespace sek