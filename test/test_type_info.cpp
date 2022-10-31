/*
 * Created by switchblade on 10/30/22.
 */

#include <core/type_info.hpp>

#include "tests.hpp"

struct test_struct
{
	constexpr static int constant = 0xaabb;

	void set_i(int value) { i = value; }

	int i = 0;
};

SEK_EXTERN_TYPE_INFO(test_struct);
SEK_EXPORT_TYPE_INFO(test_struct);

void test_type_info()
{
	using namespace sek::literals;

	static_assert(sek::type_name_v<int> == "int");
	static_assert(sek::type_name_v<void> == "void");
	static_assert(sek::type_name_v<test_struct> == "test_struct");

	SEK_ASSERT_ALWAYS(!"test_struct"_type.valid());

	const auto type = sek::type_info::reflect<test_struct>().type();

	SEK_ASSERT_ALWAYS("test_struct"_type.valid());
	SEK_ASSERT_ALWAYS("test_struct"_type == type);
	SEK_ASSERT_ALWAYS(type.name() == "test_struct");

	SEK_ASSERT_ALWAYS(!type.has_constant("constant"));
	SEK_ASSERT_ALWAYS(!type.has_constant("constant", sek::type_info::get<int>()));

	sek::type_info::reflect<test_struct>().constant<test_struct::constant>("constant");

	SEK_ASSERT_ALWAYS(type.has_constant("constant"));
	SEK_ASSERT_ALWAYS(type.has_constant("constant", sek::type_info::get<int>()));

	const auto constant = type.constant("constant");
	SEK_ASSERT_ALWAYS(!constant.empty());
	SEK_ASSERT_ALWAYS(constant.type() == sek::type_info::get<int>());
	SEK_ASSERT_ALWAYS(constant.as<const int &>() == test_struct::constant);

	const auto value0 = type.construct(std::in_place);
	SEK_ASSERT_ALWAYS(!value0.empty());
	SEK_ASSERT_ALWAYS(value0.type() == sek::type_info::get<test_struct>());

	auto i_ptr = value0.get<test_struct>();
	SEK_ASSERT_ALWAYS(i_ptr != nullptr);
	SEK_ASSERT_ALWAYS(i_ptr->i == 0);
}