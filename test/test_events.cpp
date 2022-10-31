/*
 * Created by switchblade on 10/30/22.
 */

#include <core/event.hpp>

#include "tests.hpp"

int delegate2_func() { return 2; }

void test_events()
{
	const auto delegate0 = sek::delegate{[]() -> int { return 0; }};
	SEK_ASSERT_ALWAYS(delegate0() == 0);

	const auto delegate1 = sek::delegate{+[]() -> int { return 1; }};
	SEK_ASSERT_ALWAYS(delegate1() == 1);

	const auto delegate2 = sek::delegate{sek::delegate_func<delegate2_func>};
	SEK_ASSERT_ALWAYS(delegate2() == 2);

	sek::event<bool(int, int)> event0;
	sek::event<int(int)> event1;

	sek::event_proxy<decltype(event0)> proxy0 = event0;
	sek::event_proxy<decltype(event1)> proxy1 = event1;

	auto sub0 = proxy0 += [](int a, int b) { return a == b; };
	auto sub1 = proxy1 += [](int i) { return i; };

	SEK_ASSERT_ALWAYS(proxy0.find(sub0) != proxy0.end());
	SEK_ASSERT_ALWAYS(proxy1.find(sub1) != proxy1.end());

	event0([](bool result) { SEK_ASSERT_ALWAYS(!result); }, 0, 1);
	event0([](bool result) { SEK_ASSERT_ALWAYS(result); }, 1, 1);
	event1([](int i) { SEK_ASSERT_ALWAYS(i == 0); }, 0);

	proxy0 -= sub0;
	proxy1 -= sub1;

	SEK_ASSERT_ALWAYS(proxy0.find(sub0) == proxy0.end());
	SEK_ASSERT_ALWAYS(proxy1.find(sub1) == proxy1.end());

	sek::event<void()> event2;
	std::size_t idx = 0;

	sub0 = event2 += [&idx]() { ++idx; };
	sub1 = event2 += [&idx]() { ++idx; };

	event2.subscribe_before(sub0, [&idx]() { SEK_ASSERT_ALWAYS(idx == 0); });
	event2.subscribe_after(sub0, [&idx]() { SEK_ASSERT_ALWAYS(idx == 1); });
	event2.subscribe_before(sub1, [&idx]() { SEK_ASSERT_ALWAYS(idx == 1); });
	event2.subscribe_after(sub1, [&idx]() { SEK_ASSERT_ALWAYS(idx == 2); });
	event2();
}