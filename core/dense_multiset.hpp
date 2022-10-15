//
// Created by switch_blade on 2022-10-12.
//

#pragma once

#include "detail/dense_hash_table.hpp"
#include "meta.hpp"

namespace sek
{
	namespace detail
	{
		template<typename, typename>
		struct tuple_key_hash_impl;
		template<typename H, typename... Ks>
		struct tuple_key_hash_impl<H, std::tuple<Ks...>> : std::conjunction<std::is_invocable<H, Ks>...>
		{
		};
		template<typename H, typename K>
		concept tuple_key_hash = tuple_key_hash_impl<H, K>::value;

		template<typename, typename>
		struct tuple_key_cmp_impl;
		template<typename C, typename... Ks>
		struct tuple_key_cmp_impl<C, std::tuple<Ks...>> : std::conjunction<std::is_invocable<C, Ks, std::tuple<Ks...>>...>
		{
		};
		template<typename C, typename K>
		concept tuple_key_cmp = tuple_key_cmp_impl<C, K>::value;
	}	 // namespace detail

	/** @brief Hash table used to map multiple keys to each other.
	 *
	 * A multiset is a hash table, where given an entry with value (A, B, C), such entry can be obtained
	 * via any of the keys keys A, B or C. As such, a multiset can be used to define a bidirectional
	 * relationship between two or more keys.
	 *
	 * @tparam Key Template instance of type `std::tuple`, used to define keys of the multiset.
	 * @tparam KeyHash Functor used to generate hashes for keys. By default uses `default_hash` which calls static
	 * non-member `hash` function via ADL if available, otherwise invokes `std::hash`.
	 * @tparam KeyComp Predicate used to compare keys.
	 * @tparam Alloc Allocator used for the set.
	 * @note `KeyHash` must be invocable for every key from `Key`.
	 * @note `KeyComp` must be invocable for every combination of key type from `Key` and a tuple of every key. */
	template<template_instance<std::tuple> Key,
			 detail::tuple_key_hash<Key> KeyHash = default_hash,
			 detail::tuple_key_cmp<Key> KeyComp = std::equal_to<>,
			 typename Alloc = std::allocator<Key>>
	class dense_multiset : detail::dense_table_traits<Key, KeyHash, KeyComp, forward_identity>
	{
		using table_traits = detail::dense_table_traits<Key, KeyHash, KeyComp, forward_identity>;

	public:
		typedef Key key_type;
		typedef Key value_type;
		typedef Alloc allocator_type;

		typedef typename table_traits::key_equal key_equal;
		typedef typename table_traits::hash_type hash_type;
		typedef typename table_traits::size_type size_type;
		typedef typename table_traits::difference_type difference_type;

		template<size_type N>
		using n_key_type = std::tuple_element_t<N, Key>;

	private:
		using table_traits::get_key;
		using table_traits::initial_capacity;
		using table_traits::initial_load_factor;
		using table_traits::npos;

	public:
	private:
		float m_max_load_factor = initial_load_factor;
	};
}	 // namespace sek