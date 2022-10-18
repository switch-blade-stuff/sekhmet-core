/*
 * Created by switchblade on 2022-10-18.
 */

#include "../assert.hpp"

#include <cstdio>

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