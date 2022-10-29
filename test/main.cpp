/*
 * Created by switchblade on 10/29/22.
 */

#include <cstring>

#include "tests.hpp"

int main(int argc, char *argv[])
{
	for (auto i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "dense_map")) test_dense_map();
		if (!strcmp(argv[i], "dense_set")) test_dense_set();
		if (!strcmp(argv[i], "dense_multiset")) test_dense_multiset();
	}
}