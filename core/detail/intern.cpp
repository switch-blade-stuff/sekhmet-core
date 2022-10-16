/*
 * Created by switchblade on 10/05/22
 */

#include "../interned_string.hpp"

namespace sek
{
	template SEK_API_EXPORT basic_intern_pool<char> &basic_intern_pool<char>::global();
	template SEK_API_EXPORT basic_intern_pool<wchar_t> &basic_intern_pool<wchar_t>::global();
}	 // namespace sek