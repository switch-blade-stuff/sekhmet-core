/*
 * Created by switchblade on 10/29/22.
 */

#include <cstring>

#include "tests.hpp"

int main(int argc, char *argv[])
{
	for (auto i = 1; i < argc; ++i)
	{
		for (auto test : test_funcs)
			if (test.first == argv[i]) test.second();
	}
}