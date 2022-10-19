/*
 * Created by switchblade on 2022-10-18.
 */

#include "../assert.hpp"

#include <cstdio>
#include <cstdlib>

#if defined(__has_builtin) && !defined(__ibmxl__)
#if __has_builtin(__builtin_debugtrap)
void sek::detail::debug_trap() noexcept { __builtin_debugtrap(); }
#elif __has_builtin(__debugbreak)
void sek::detail::debug_trap() noexcept { __debugbreak(); }
#endif
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
void sek::detail::debug_trap() noexcept { __debugbreak(); }
#elif defined(__ARMCC_VERSION)
void sek::detail::debug_trap() noexcept { __breakpoint(42); }
#elif defined(__ibmxl__) || defined(__xlC__)
#include <builtins.h>
void sek::detail::debug_trap() noexcept { __trap(42); }
#elif defined(__DMC__) && defined(_M_IX86)
void sek::detail::debug_trap() noexcept { __asm int 3h; }
#elif defined(__i386__) || defined(__x86_64__)
void sek::detail::debug_trap() noexcept { __asm__ __volatile__("int3"); }
#error
#elif defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 0) && defined(__GNUC__)
void sek::detail::debug_trap() noexcept { __builtin_trap(); }
#else
#include <signal.h>
#if defined(SIGTRAP)
void sek::detail::debug_trap() noexcept { raise(SIGTRAP); }
#else
void sek::detail::debug_trap() noexcept { raise(SIGABRT); }
#endif
#endif

namespace sek::detail
{
	void assert_unreachable(const char *file, std::size_t line, const char *func)
	{
		fprintf(stderr, "Reached unreachable code at '%s:%lu' in '%s'\n", file, line, func);
		std::abort();
	}
	void assert_fail(const char *file, std::size_t line, const char *func, const char *cond_str, const char *msg)
	{
		fprintf(stderr, "Assertion ");
		if (cond_str) [[likely]]
			fprintf(stderr, "(%s) ", cond_str);

		fprintf(stderr, "failed at '%s:%lu' in '%s'", file, line, func);
		if (msg) [[likely]]
			fprintf(stderr, ": %s", msg);
		fputc('\n', stderr);

		std::abort();
	}
}	 // namespace sek::detail