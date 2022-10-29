/*
 * Created by switchblade on 10/29/22.
 */

#include <core/dense_map.hpp>

#include "tests.hpp"
#include <string_view>

void test_dense_map()
{
	sek::dense_map<std::string, std::string> map;

	SEK_ASSERT_ALWAYS(map.empty());
	SEK_ASSERT_ALWAYS(map.size() == 0);
	SEK_ASSERT_ALWAYS(map.bucket_count() != 0);
	SEK_ASSERT_ALWAYS(map.load_factor() == 0.0f);

	SEK_ASSERT_ALWAYS(!map.contains("key0"));

	const auto insert0 = map.emplace("key0", "value0");
	SEK_ASSERT_ALWAYS(insert0.second == true);
	SEK_ASSERT_ALWAYS(insert0.first != map.end());

	SEK_ASSERT_ALWAYS(map.contains("key0"));
	SEK_ASSERT_ALWAYS(map.find("key0") == insert0.first);
	SEK_ASSERT_ALWAYS(map.at("key0") == "value0");
	SEK_ASSERT_ALWAYS(map.find("key0")->second == "value0");

	const auto insert1 = map.insert({"key0", "value1"});
	SEK_ASSERT_ALWAYS(insert1.second == false);
	SEK_ASSERT_ALWAYS(insert1.first == insert0.first);
	SEK_ASSERT_ALWAYS(map.at("key0") == "value1");

	const auto insert2 = map.insert({"key1", "value1"});
	SEK_ASSERT_ALWAYS(insert2.second == true);
	SEK_ASSERT_ALWAYS(insert2.first != map.end());
	SEK_ASSERT_ALWAYS(map.at("key1") == "value1");

	SEK_ASSERT_ALWAYS(map.contains("key1"));
	SEK_ASSERT_ALWAYS(map.erase("key1"));
	SEK_ASSERT_ALWAYS(!map.contains("key1"));
	SEK_ASSERT_ALWAYS(!map.erase("key1"));

	SEK_ASSERT_ALWAYS(!map.try_emplace("key0", "value0").second);
	SEK_ASSERT_ALWAYS(map.try_emplace("key1", "value1").second);

	SEK_ASSERT_ALWAYS(!map.empty());
	map.clear();
	SEK_ASSERT_ALWAYS(map.empty());

	const std::size_t count = 1000;
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto value = fmt::format("value{}", i);
		const auto key = fmt::format("key{}", i);

		const auto insert_i = map.insert({key, value});
		SEK_ASSERT_ALWAYS(insert_i.second == true);
		SEK_ASSERT_ALWAYS(insert_i.first != map.end());
		SEK_ASSERT_ALWAYS(map.contains(key));
		SEK_ASSERT_ALWAYS(map.at(key) == value);
	}

	SEK_ASSERT_ALWAYS(map.size() == count);
	map.clear();
	SEK_ASSERT_ALWAYS(map.size() == 0);
}