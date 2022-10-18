/*
 * Created by switchblade on 23/09/22
 */

#pragma once

#include <utility>

#include "../define.h"

namespace sek::detail
{
#ifndef __cpp_lib_unreachable
	[[noreturn]] SEK_CORE_PUBLIC void assert_unreachable(const char *file, std::size_t line, const char *func);
#endif

	inline void constexpr_assert_fail(const char *msg) { throw msg; }
	SEK_CORE_PUBLIC void assert_fail(const char *file, std::size_t line, const char *func, const char *cond_str, const char *msg);
	constexpr void assert(bool cond, const char *file, std::size_t line, const char *func, const char *cond_str, const char *msg)
	{
		if (!cond) [[unlikely]]
		{
			if (!std::is_constant_evaluated())
				assert_fail(file, line, func, cond_str, msg);
			else
				constexpr_assert_fail(msg);
		}
	}
}	 // namespace sek::detail

#define SEK_ASSERT_2(cnd, msg) sek::detail::assert((cnd), (SEK_FILE), (SEK_LINE), (SEK_PRETTY_FUNC), (#cnd), (msg))
#define SEK_ASSERT_1(cnd) SEK_ASSERT_2(cnd, nullptr)

/** Same as regular SEK_ASSERT, except applies even when SEK_NO_DEBUG_ASSERT is defined. */
#define SEK_ASSERT_ALWAYS(...) SEK_GET_MACRO_2(__VA_ARGS__, SEK_ASSERT_2, SEK_ASSERT_1)(__VA_ARGS__)

/** Asserts that the code should never be reached. */
#ifndef __cpp_lib_unreachable
#define SEK_NEVER_REACHED sek::detail::assert_unreachable((SEK_FILE), (SEK_LINE), (SEK_PRETTY_FUNC))
#else
#define SEK_NEVER_REACHED std::unreachable();
#endif

#if !defined(SEK_NO_DEBUG_ASSERT) && defined(SEK_DEBUG)
/** Assert that supports an optional message, prints the enclosing function name and terminates using exit(1).
 * @note Currently the message can only be a char literal. */
#define SEK_ASSERT(...) SEK_ASSERT_ALWAYS(__VA_ARGS__)
#else
#define SEK_ASSERT(...)
#endif
