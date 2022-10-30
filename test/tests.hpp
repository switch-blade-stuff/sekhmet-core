/*
 * Created by switchblade on 10/29/22.
 */

#pragma once

#include <core/assert.hpp>

#include <fmt/format.h>
#include <string_view>

void test_events();

void test_dense_map();
void test_dense_set();
void test_dense_multiset();

static std::pair<std::string_view, void (*)()> test_funcs[] = {
	{"events", test_events},
	{"dense_map", test_dense_map},
	{"dense_set", test_dense_set},
	{"dense_multiset", test_dense_multiset},
};
