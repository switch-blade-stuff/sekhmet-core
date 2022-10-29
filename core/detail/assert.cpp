/*
 * Created by switchblade on 2022-10-18.
 */

#include "../assert.hpp"

#include <cstdio>
#include <exception>

namespace sek::detail
{
	void print_assert(const char *file, std::size_t line, const char *func, const char *cstr, const char *msg) noexcept
	{
		fprintf(stderr, "Assertion ");
		if (cstr) [[likely]]
			fprintf(stderr, "(%s) ", cstr);

		fprintf(stderr, "failed at '%s:%lu' in '%s'", file, line, func);
		if (msg) [[likely]]
			fprintf(stderr, ": %s", msg);
		fputc('\n', stderr);
	}
	void print_unreachable(const char *file, std::size_t line, const char *func) noexcept
	{
		fprintf(stderr, "Unreachable code at '%s:%lu' in '%s'\n", file, line, func);
	}

	void assert_fail() noexcept { std::terminate(); }
}	 // namespace sek::detail