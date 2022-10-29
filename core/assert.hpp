/*
 * Created by switchblade on 23/09/22
 */

#pragma once

#include <utility>

#include "define.h"

#if defined(__has_builtin) && !defined(__ibmxl__)
#if __has_builtin(__builtin_debugtrap)
#define SEK_DEBUG_TRAP_ALWAYS() __builtin_debugtrap()
#elif __has_builtin(__debugbreak)
#define SEK_DEBUG_TRAP_ALWAYS() __debugbreak()
#endif
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define SEK_DEBUG_TRAP_ALWAYS() __debugbreak()
#elif defined(__ARMCC_VERSION)
#define SEK_DEBUG_TRAP_ALWAYS() __breakpoint(42)
#elif defined(__ibmxl__) || defined(__xlC__)
#include <builtins.h>
#define SEK_DEBUG_TRAP_ALWAYS() __trap(42)
#elif defined(__DMC__) && defined(_M_IX86)
#define SEK_DEBUG_TRAP_ALWAYS() (__asm int 3h)
#elif defined(__i386__) || defined(__x86_64__)
#define SEK_DEBUG_TRAP_ALWAYS() (__asm__ __volatile__("int3"))
#elif defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 0) && defined(__GNUC__)
#define SEK_DEBUG_TRAP_ALWAYS() __builtin_trap()
#endif

#ifndef SEK_DEBUG_TRAP_ALWAYS
#include <csignal>
#if defined(SIGTRAP)
#define SEK_DEBUG_TRAP_ALWAYS() raise(SIGTRAP)
#else
#define SEK_DEBUG_TRAP_ALWAYS() raise(SIGABRT)
#endif
#endif

namespace sek::detail
{
	SEK_CORE_PUBLIC void print_assert(const char *file, std::size_t line, const char *func, const char *cstr, const char *msg) noexcept;
	SEK_CORE_PUBLIC void print_unreachable(const char *file, std::size_t line, const char *func) noexcept;

	[[noreturn]] inline void assert_fail_constexpr(const char *msg) { throw msg; }
	[[noreturn]] SEK_CORE_PUBLIC void assert_fail() noexcept;

	[[noreturn]] SEK_FORCE_INLINE void unreachable([[maybe_unused]] const char *file,
												   [[maybe_unused]] std::size_t line,
												   [[maybe_unused]] const char *func)
	{
#ifdef SEK_DEBUG
		print_unreachable(file, line, func);
#endif

#if defined(__cpp_lib_unreachable)
		std::unreachable();
#elif defined(__GNUC__)
		__builtin_unreachable();
#elif defined(_MSC_VER)
		__assume(false);
#else
		assert_fail();
#endif
	}

	SEK_FORCE_INLINE constexpr void assert([[maybe_unused]] bool cnd,
										   [[maybe_unused]] const char *file,
										   [[maybe_unused]] std::size_t line,
										   [[maybe_unused]] const char *func,
										   [[maybe_unused]] const char *cstr,
										   [[maybe_unused]] const char *msg)
	{
		if (!cnd) [[unlikely]]
		{
			if (!std::is_constant_evaluated())
			{
				print_assert(file, line, func, cstr, msg);
				SEK_DEBUG_TRAP_ALWAYS();
				assert_fail();
			}
			else
				assert_fail_constexpr(msg);
		}
	}
}	 // namespace sek::detail

#define SEK_ASSERT_2(cnd, msg) sek::detail::assert((cnd), (SEK_FILE), (SEK_LINE), (SEK_PRETTY_FUNC), (#cnd), (msg))
#define SEK_ASSERT_1(cnd) SEK_ASSERT_2(cnd, nullptr)

/** Same as regular SEK_ASSERT, except applies even when SEK_NO_DEBUG_ASSERT is defined. */
#define SEK_ASSERT_ALWAYS(...) SEK_GET_MACRO_2(__VA_ARGS__, SEK_ASSERT_2, SEK_ASSERT_1)(__VA_ARGS__)
/** Asserts that the code should never be reached. */
#define SEK_NEVER_REACHED sek::detail::unreachable((SEK_FILE), (SEK_LINE), (SEK_PRETTY_FUNC))

#if !defined(SEK_NO_DEBUG_ASSERT) && defined(SEK_DEBUG)
/** Assert that supports an optional message, prints the enclosing function name and terminates using exit(1).
 * @note Currently the message can only be a char literal. */
#define SEK_ASSERT(...) SEK_ASSERT_ALWAYS(__VA_ARGS__)
#define SEK_DEBUG_TRAP() SEK_DEBUG_TRAP_ALWAYS()
#else
#define SEK_ASSERT(...)
#define SEK_DEBUG_TRAP()
#endif
