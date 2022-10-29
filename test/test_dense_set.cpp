/*
 * Created by switchblade on 10/29/22.
 */

#include <core/dense_set.hpp>

#include "tests.hpp"

void test_dense_set()
{
	sek::dense_set<std::string> set;

	SEK_ASSERT_ALWAYS(set.empty());
	SEK_ASSERT_ALWAYS(set.size() == 0);
	SEK_ASSERT_ALWAYS(set.bucket_count() != 0);
	SEK_ASSERT_ALWAYS(set.load_factor() == 0.0f);

	SEK_ASSERT_ALWAYS(!set.contains("key0"));

	const auto insert0 = set.emplace("key0");
	SEK_ASSERT_ALWAYS(insert0.second == true);
	SEK_ASSERT_ALWAYS(insert0.first != set.end());

	SEK_ASSERT_ALWAYS(set.contains("key0"));
	SEK_ASSERT_ALWAYS(set.find("key0") == insert0.first);

	const auto insert1 = set.insert("key0");
	SEK_ASSERT_ALWAYS(insert1.second == false);
	SEK_ASSERT_ALWAYS(insert1.first == insert0.first);

	const auto insert2 = set.insert("key1");
	SEK_ASSERT_ALWAYS(insert2.second == true);
	SEK_ASSERT_ALWAYS(insert2.first != set.end());

	SEK_ASSERT_ALWAYS(set.contains("key1"));
	SEK_ASSERT_ALWAYS(set.erase("key1"));
	SEK_ASSERT_ALWAYS(!set.contains("key1"));
	SEK_ASSERT_ALWAYS(!set.erase("key1"));

	SEK_ASSERT_ALWAYS(!set.try_insert("key0").second);
	SEK_ASSERT_ALWAYS(set.try_insert("key1").second);

	SEK_ASSERT_ALWAYS(!set.empty());
	set.clear();
	SEK_ASSERT_ALWAYS(set.empty());

	const std::size_t count = 1000;
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto key = fmt::format("key{}", i);

		const auto insert_i = set.insert(key);
		SEK_ASSERT_ALWAYS(insert_i.second == true);
		SEK_ASSERT_ALWAYS(insert_i.first != set.end());
		SEK_ASSERT_ALWAYS(set.contains(key));
	}

	SEK_ASSERT_ALWAYS(set.size() == count);
	set.clear();
	SEK_ASSERT_ALWAYS(set.size() == 0);
}