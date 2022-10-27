/*
 * Created by switchblade on 2022-10-04.
 */

#include "type_info.hpp"

SEK_EXPORT_TYPE_INFO(sek::any)
SEK_EXPORT_TYPE_INFO(sek::type_info)

namespace sek
{
	bool type_info::inherits(type_info type) const noexcept
	{
		for (auto &parent : parents())
		{
			const auto parent_type = parent.type();
			if (parent_type == type || parent_type.inherits(type)) [[likely]]
				return true;
		}
		return false;
	}
}	 // namespace sek
