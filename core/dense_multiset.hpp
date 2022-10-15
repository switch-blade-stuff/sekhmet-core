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
	 * @note `KeyHash` and `KeyComp` must be invocable for every key type from the `Key` tuple. */
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

		/** Size of the `key_type` tuple. */
		constexpr static size_type key_size = std::tuple_size_v<Key>;

	private:
		using table_traits::get_key;
		using table_traits::initial_capacity;
		using table_traits::initial_load_factor;
		using table_traits::npos;

		template<size_type I>
		using n_key_type = std::tuple_element<I, key_type>;
		template<size_type I>
		using n_key_ref = const std::remove_cvref_t<n_key_type<I>> &;

		class sparse_entry
		{
		public:
			constexpr sparse_entry() noexcept = default;
			constexpr explicit sparse_entry(size_type value) noexcept
			{
				std::fill(m_values.begin(), m_values.end(), value);
			}

			[[nodiscard]] constexpr auto &operator[](size_type i) noexcept { return m_values[i]; }
			[[nodiscard]] constexpr auto &operator[](size_type i) const noexcept { return m_values[i]; }

			constexpr void swap(sparse_entry &other) noexcept { std::swap(m_values, other.m_values); }
			friend constexpr void swap(sparse_entry &a, sparse_entry &b) noexcept { a.swap(b); }

		private:
			std::array<size_type, key_size> m_values = {};
		};

		using sparse_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<sparse_entry>;
		using sparse_data = std::vector<sparse_entry, sparse_alloc>;

		class dense_entry
		{
		public:
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;

			constexpr static size_type npos = std::numeric_limits<size_type>::max();

		public:
			constexpr dense_entry() = default;
			constexpr dense_entry(const dense_entry &) = default;
			constexpr dense_entry &operator=(const dense_entry &) = default;
			constexpr dense_entry(dense_entry &&) noexcept(std::is_nothrow_move_constructible_v<value_type>) = default;
			constexpr dense_entry &operator=(dense_entry &&) noexcept(std::is_nothrow_move_assignable_v<value_type>) = default;
			constexpr ~dense_entry() = default;

			// clang-format off
			template<typename... Args>
			constexpr explicit dense_entry(Args &&...args) requires std::constructible_from<value_type, Args...>
				: value(std::forward<Args>(args)...) {}
			// clang-format on

			template<size_type I>
			[[nodiscard]] constexpr n_key_ref<I> key() const noexcept
			{
				using std::get;
				return get<I>(value);
			}

			constexpr void swap(dense_entry &other) noexcept(std::is_nothrow_swappable_v<value_type>)
			{
				using std::swap;
				swap(value, other.value);
				swap(next, other.next);
				swap(hash, other.hash);
			}
			friend constexpr void swap(dense_entry &a, dense_entry &b) noexcept(std::is_nothrow_swappable_v<value_type>)
			{
				a.swap(b);
			}

			value_type value;

			std::array<size_type, key_size> next = {};
			std::array<hash_t, key_size> hash = {};
		};

		using dense_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<dense_entry>;
		using dense_data = std::vector<dense_entry, dense_alloc>;

	public:
		[[nodiscard]] constexpr auto allocator() const noexcept { return value_vector().get_allocator(); }

		constexpr void swap(dense_multiset &other) noexcept
		{
			using std::swap;
			swap(m_sparse_data, other.m_sparse_data);
			swap(m_dense_data, other.m_dense_data);
			swap(m_max_load_factor, other.m_max_load_factor);
		}
		friend constexpr void swap(dense_multiset &a, dense_multiset &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr auto &value_vector() noexcept { return m_dense_data.first(); }
		[[nodiscard]] constexpr const auto &value_vector() const noexcept { return m_dense_data.first(); }
		[[nodiscard]] constexpr auto &bucket_vector() noexcept { return m_sparse_data.first(); }
		[[nodiscard]] constexpr const auto &bucket_vector() const noexcept { return m_sparse_data.first(); }

		[[nodiscard]] constexpr auto &get_hash() const noexcept { return m_sparse_data.second(); }
		[[nodiscard]] constexpr auto &get_comp() const noexcept { return m_dense_data.second(); }

		packed_pair<dense_data, key_equal> m_dense_data;
		packed_pair<sparse_data, hash_type> m_sparse_data = {sparse_data(initial_capacity, sparse_entry{npos}), hash_type{}};

		float m_max_load_factor = initial_load_factor;
	};
}	 // namespace sek