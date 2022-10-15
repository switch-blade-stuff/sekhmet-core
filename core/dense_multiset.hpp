/*
 * Created by switch_blade on 2022-10-12.
 */

#pragma once

#include "detail/dense_hash_table.hpp"
#include "meta.hpp"

namespace sek
{
	/** @brief Hash table used to map multiple keys to each other.
	 *
	 * A multiset is a hash table, where given an entry with value `std::tuple<A, B, C>`, such entry can be obtained
	 * via any of the keys keys `A`, `B` or `C`. As such, a multiset can be used to define a multidirectional
	 * relationship between two or more keys.
	 *
	 * @tparam Key Template instance of a tuple-like type, used to define keys of the multiset.
	 * @tparam KeyHash Functor used to generate hashes for keys. By default uses `default_hash` which calls static
	 * non-member `hash` function via ADL if available, otherwise invokes `std::hash`.
	 * @tparam KeyComp Predicate used to compare keys.
	 * @tparam Alloc Allocator used for the set.
	 * @note `KeyHash` must be invocable for every key from `Key`.
	 * @note `KeyComp` must be invocable for every combination of key type from `Key` and a tuple of every key. */
	template<tuple_like Key, typename KeyHash = default_hash, typename KeyComp = std::equal_to<>, typename Alloc = std::allocator<Key>>
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

		/** Alias for the `N`th key type from the `key_type` tuple. */
		template<size_type N>
		using n_key_type = std::tuple_element_t<N, Key>;
		/** Size of the `key_type` tuple. */
		constexpr static size_type key_size = std::tuple_size_v<Key>;

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