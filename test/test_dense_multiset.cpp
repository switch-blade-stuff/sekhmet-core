/*
 * Created by switchblade on 10/29/22.
 */

#include <core/hash.hpp>

#include <core/dense_multiset.hpp>
#include <core/uuid.hpp>

#include "tests.hpp"

void test_dense_multiset()
{
	sek::dense_multiset<std::pair<std::string, sek::uuid>> set;

	const auto id0 = sek::uuid(sek::uuid::version4{});
	const auto id1 = sek::uuid(sek::uuid::version4{});

	SEK_ASSERT_ALWAYS(set.empty());
	SEK_ASSERT_ALWAYS(set.size() == 0);
	SEK_ASSERT_ALWAYS(set.bucket_count() != 0);
	SEK_ASSERT_ALWAYS(set.load_factor() == 0.0f);

	SEK_ASSERT_ALWAYS(!set.contains<0>("key0"));
	SEK_ASSERT_ALWAYS(!set.contains<1>(id0));

	const auto insert0 = set.emplace("key0", id0);
	SEK_ASSERT_ALWAYS(insert0.second == 0);
	SEK_ASSERT_ALWAYS(insert0.first != set.end());

	SEK_ASSERT_ALWAYS(set.contains<0>("key0"));
	SEK_ASSERT_ALWAYS(set.find<0>("key0") == insert0.first);
	SEK_ASSERT_ALWAYS(set.contains<1>(id0));
	SEK_ASSERT_ALWAYS(set.find<1>(id0) == insert0.first);

	const auto insert1 = set.insert({"key0", id1});
	SEK_ASSERT_ALWAYS(insert1.second == 1);
	SEK_ASSERT_ALWAYS(!set.contains<1>(id0));
	SEK_ASSERT_ALWAYS(set.contains<1>(id1));
	SEK_ASSERT_ALWAYS(set.find<1>(id1) == insert1.first);

	const auto insert2 = set.insert({"key1", id1});
	SEK_ASSERT_ALWAYS(insert2.second == 1);
	SEK_ASSERT_ALWAYS(insert2.first != set.end());
	SEK_ASSERT_ALWAYS(set.contains<1>(id1));
	SEK_ASSERT_ALWAYS(set.find<1>(id1) == insert2.first);

	SEK_ASSERT_ALWAYS(!set.contains<0>("key0"));
	SEK_ASSERT_ALWAYS(set.contains<0>("key1"));
	SEK_ASSERT_ALWAYS(set.erase<0>("key1"));
	SEK_ASSERT_ALWAYS(!set.contains<0>("key1"));
	SEK_ASSERT_ALWAYS(!set.erase<0>("key1"));

	SEK_ASSERT_ALWAYS(set.empty());

	const std::size_t count = 1000;
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto key0 = fmt::format("key{}", i);
		const auto key1 = sek::uuid{sek::uuid::version4{}};

		const auto insert_i = set.insert({key0, key1});
		SEK_ASSERT_ALWAYS(insert_i.second == 0);
		SEK_ASSERT_ALWAYS(insert_i.first != set.end());
		SEK_ASSERT_ALWAYS(set.contains<0>(key0));
		SEK_ASSERT_ALWAYS(set.contains<1>(key1));
	}

	SEK_ASSERT_ALWAYS(set.size() == count);
	set.clear();
	SEK_ASSERT_ALWAYS(set.size() == 0);
}