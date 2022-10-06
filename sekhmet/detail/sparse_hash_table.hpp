/*
 * Created by switchblade on 2021-12-07
 */

#pragma once

#include <algorithm>

#include "../assert.hpp"
#include "../hash.hpp"
#include "../math/utility.hpp"
#include "alloc_util.hpp"
#include "ebo_base_helper.hpp"
#include "flagged_ptr.hpp"
#include "table_util.hpp"

namespace sek::detail
{
	/**
	 * Hash table is implemented as list of objects (array of pointers) indexed by open-addressing. The reason list is
	 * used over a forward list, is that a linked list has the worst locality and requires expensive lookup of buckets
	 * during erase and potentially insert operations. In addition to that, if a forward list was used,
	 * separate array of buckets would be required, which would waste memory.nn
	 *
	 * While using a list requires skipping empty buckets during iteration, this is relatively inexpensive,
	 * since all pointers in a list are located in the same array, thus skipping multiple pointers would take
	 * advantage of them being cached.nn
	 *
	 * I decided not to use an array (ex. how Google's densehash does), since while it would provide the most fastest cache
	 * performance, it would also create a lot of wasted space for large objects. Another determent to using an array,
	 * is that you can't simply check if a node is empty by comparing it to nullptr without storing an additional flag
	 * alongside the value. To avoid that you would need to store an additional default-constructed key that you would
	 * then compare to, which would mean the keys you use cannot be default-constructed or be equal to default-constructed
	 * keys. This especially is bad when you are using things like strings, integers, or really any type whose default
	 * value can have a meaning and could be used as a key. For example, you would not be able to have a set that looks
	 * like this {"", "a", "B"}, since the empty string "", would be equal to a default-constructed string,
	 * and thus would be treated as an empty node. Same issue happens with "tombstone" buckets.
	 *
	 * Using a list would provide a better overall performance than using a linked list,
	 * while being more conservative about the amount of memory allocated.
	 * In addition, the indirection caused by using a list in place of an array can be avoided
	 * by storing hash of the key alongside the pointer within the bucket. The "immediate" hash would be used
	 * for rough comparison (if hashes compare equal, the keys are most likely to be equal too).
	 * And only if the hashes compare equal a key comparison (and thus an indirection) would be needed (since there could be hash contention).
	 * This way bucket search operations would take advantage of the cached array without storing values in-place.nn
	 *
	 * Storing an in-place hash would only have overhead of sizeof(std::size_t) and no runtime calculation overhead,
	 * since hash calculation is already required for bucket lookup.nn
	 *
	 * A disadvantage to using a single open addressed array over a linked list and array of buckets is that you loose
	 * the ability to have multimaps, since they require multiple nodes per node.
	 * That however is not that big of an issue, since you can implement a miltimap as a map of vectors/lists/arrays.\n\n
	 *
	 * A possible implementation of a multimap would be to have each node be an independently linked list, but that would
	 * just return us to using linked lists, and would additionally require complex iteration, since the nodes would not
	 * be contiguous (you'd need to iterate not only over buckets but also over nodes of each node). You'd need to track
	 * both your position in the table and within the node, which if you implement multimap as map of vectors, you'd
	 * need to do the same, but will avoid the use of linked lists.
	 * A multiset can be easily implemented as a map of counters that would be used to track the amount of specific keys within the set.
	 * */
	template<typename KeyType, typename ValueType, typename KeyHash, typename KeyCompare, typename KeyExtract, typename Allocator>
	class sparse_hash_table;

	template<typename KeyType, typename ValueType, typename KeyExtract>
	struct sparse_table_bucket
	{
		[[nodiscard]] constexpr static auto tombstone_ptr() noexcept
		{
			return std::bit_cast<ValueType *>(std::numeric_limits<std::intptr_t>::max());
		}

		constexpr sparse_table_bucket() noexcept = default;
		constexpr explicit sparse_table_bucket(ValueType *ptr, hash_t hash) noexcept : hash(hash), data(ptr) {}

		[[nodiscard]] constexpr bool is_empty() const noexcept { return data == nullptr; }
		[[nodiscard]] constexpr bool is_tombstone() const noexcept { return data == tombstone_ptr(); }
		[[nodiscard]] constexpr bool is_occupied() const noexcept { return !is_empty() && !is_tombstone(); }
		constexpr void set_empty() noexcept { data = nullptr; }
		constexpr void set_tombstone() noexcept { data = tombstone_ptr(); }

		[[nodiscard]] constexpr decltype(auto) value() const noexcept { return *data; }
		[[nodiscard]] constexpr decltype(auto) key() const noexcept { return KeyExtract{}(value()); }

		constexpr void swap(sparse_table_bucket &other) noexcept
		{
			using std::swap;
			swap(hash, other.hash);
			swap(data, other.data);
		}

		hash_t hash = 0;
		ValueType *data;
	};

	template<typename KeyType, typename ValueType, typename KeyHash, typename KeyCompare, typename KeyExtract, typename Allocator>
	class sparse_hash_table
		: ebo_base_helper<Allocator>,
		  ebo_base_helper<rebind_alloc_t<Allocator, sparse_table_bucket<KeyType, ValueType, KeyExtract>>>,
		  ebo_base_helper<KeyCompare>,
		  ebo_base_helper<KeyHash>
	{
		using bucket_type = sparse_table_bucket<KeyType, ValueType, KeyExtract>;

	public:
		typedef KeyType key_type;
		typedef ValueType value_type;
		typedef KeyCompare key_equal;
		typedef Allocator allocator_type;
		typedef rebind_alloc_t<Allocator, bucket_type> bucket_allocator_type;
		typedef KeyHash hash_type;

		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	private:
		using alloc_traits = std::allocator_traits<allocator_type>;
		using value_ebo_base = ebo_base_helper<allocator_type>;
		using bucket_alloc_traits = std::allocator_traits<bucket_allocator_type>;
		using bucket_ebo_base = ebo_base_helper<bucket_allocator_type>;

		using compare_ebo_base = ebo_base_helper<key_equal>;
		using hash_ebo_base = ebo_base_helper<hash_type>;

		template<bool IsConst>
		class sparse_table_iterator
		{
			template<bool B>
			friend class hash_table_iterator;

			friend class sparse_hash_table;

		public:
			typedef std::conditional_t<IsConst, const ValueType, ValueType> value_type;
			typedef value_type *pointer;
			typedef value_type &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			using bucket_ptr_type = bucket_type *;

			constexpr explicit sparse_table_iterator(bucket_ptr_type current, bucket_ptr_type end) noexcept
				: m_bucket_ptr(current), m_end_ptr(end)
			{
				skip_to_next_occupied();
			}

		public:
			constexpr sparse_table_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr sparse_table_iterator(const sparse_table_iterator<OtherConst> &other) noexcept
				: sparse_table_iterator(other.m_bucket_ptr, other.m_end_ptr)
			{
			}

			constexpr sparse_table_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr sparse_table_iterator &operator++() noexcept
			{
				++m_bucket_ptr;
				skip_to_next_occupied();
				return *this;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return &m_bucket_ptr->value(); }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const sparse_table_iterator &other) const noexcept
			{
				return m_bucket_ptr <=> other.m_bucket_ptr;
			}
			[[nodiscard]] constexpr bool operator==(const sparse_table_iterator &) const noexcept = default;

			friend constexpr void swap(sparse_table_iterator &a, sparse_table_iterator &b) noexcept
			{
				using std::swap;
				swap(a.m_bucket_ptr, b.m_bucket_ptr);
				swap(a.m_end_ptr, b.m_end_ptr);
			}

		private:
			constexpr void skip_to_next_occupied() noexcept
			{
				while (m_bucket_ptr != m_end_ptr && !m_bucket_ptr->is_occupied()) ++m_bucket_ptr;
			}

			bucket_ptr_type m_bucket_ptr;
			bucket_ptr_type m_end_ptr;
		};

		constexpr static float initial_load_factor = .65f;
		constexpr static float initial_tombstone_factor = .36f;
		constexpr static size_type initial_capacity = 8;

	public:
		typedef sparse_table_iterator<false> iterator;
		typedef sparse_table_iterator<true> const_iterator;

		class node_handle : ebo_base_helper<allocator_type>
		{
			friend class sparse_hash_table;

			using ebo_base = ebo_base_helper<allocator_type>;

			constexpr explicit node_handle(bucket_type &b, const allocator_type &a) noexcept : ebo_base(a), m_bucket(b)
			{
				b.set_tombstone();
			}

		public:
			node_handle(const node_handle &) = delete;
			node_handle &operator=(const node_handle &) = delete;

			constexpr node_handle() noexcept = default;
			constexpr ~node_handle() { destroy(); }

			constexpr node_handle(node_handle &&other) noexcept
				: ebo_base(std::move(other.get_allocator())), m_bucket(other.reset())
			{
			}
			constexpr node_handle &operator=(node_handle &&other) noexcept
			{
				destroy();
				get_allocator() = std::move(other.get_allocator());
				m_bucket = other.reset();
				return *this;
			}

			[[nodiscard]] constexpr bool empty() const noexcept { return !m_bucket.is_occupied(); }
			[[nodiscard]] constexpr value_type &value() const noexcept { return m_bucket.value(); }

			[[nodiscard]] constexpr allocator_type &get_allocator() noexcept { return *ebo_base::get(); }
			[[nodiscard]] constexpr const allocator_type &get_allocator() const noexcept { return *ebo_base::get(); }

			constexpr void swap(node_handle &other) noexcept
			{
				using std::swap;
				ebo_base::swap(other);
				swap(m_bucket, other.m_bucket);
			}

			friend constexpr void swap(node_handle &a, node_handle &b) noexcept { a.swap(b); }

		private:
			constexpr void destroy()
			{
				if (!empty())
				{
					std::destroy_at(&m_bucket.value());
					get_allocator().deallocate(&m_bucket.value(), 1);
				}
			}
			constexpr bucket_type reset() noexcept { return std::exchange(m_bucket, {}); }

			bucket_type m_bucket = {};
		};

	private:
		[[nodiscard]] constexpr static decltype(auto) key_extract(const value_type &value) noexcept
		{
			return KeyExtract{}(value);
		}

		[[nodiscard]] constexpr static size_type next_probe_index(size_type index, size_type i, size_type m) noexcept
		{
			return (index + i / 2 + (i * i) / 2) % m;
		}

		template<bool RequireOccupied>
		[[nodiscard]] constexpr static bucket_type *
			find_bucket_impl(bucket_type *data, size_type capacity, const auto &key, auto hash, const key_equal &compare) noexcept
		{
			if (capacity != 0) [[likely]] /* Initially, capacity is 0, so need to check. */
			{
				size_type index = static_cast<size_type>(hash % capacity), i = 0;
				for (; i < capacity; index = next_probe_index(index, ++i, capacity))
				{
					auto &bucket = data[index];
					if constexpr (!RequireOccupied)
					{
						if (!bucket.is_occupied()) return &bucket;
					}
					else
					{
						/* If we are not allowed to have empty buckets,
						 * tombstones need to be skipped and empty buckets
						 * should return end. */
						if (bucket.is_empty())
							break;
						else if (bucket.is_tombstone())
							continue;
					}

					if (bucket.hash != hash) [[likely]] /* If hashes don't match, keys are definitely different. */
						continue;
					else if (compare(key, bucket.key()))
						return &bucket; /* Keys can still be different, but we expect the hash function to be good so if hashes match, keys are likely to match too. */
				}
			}

			return data + capacity;
		}

	public:
		constexpr sparse_hash_table() noexcept(nothrow_alloc_default_construct<allocator_type, bucket_allocator_type>) = default;

		constexpr sparse_hash_table(size_type capacity, const key_equal &key_compare, const hash_type &key_hash, const allocator_type &alloc)
			: value_ebo_base(alloc),
			  bucket_ebo_base(bucket_allocator_type{alloc}),
			  compare_ebo_base(key_compare),
			  hash_ebo_base(key_hash)
		{
			if (capacity) [[likely]]
				m_buckets_data = allocate_buckets(m_buckets_capacity = next_pow_2(capacity));
		}

		constexpr sparse_hash_table(const sparse_hash_table &other)
			: value_ebo_base(make_alloc_copy(other.get_allocator())),
			  bucket_ebo_base(make_alloc_copy(other.get_bucket_allocator())),
			  compare_ebo_base(other.get_comp()),
			  hash_ebo_base(other.get_hash())
		{
			insert(other.begin(), other.end());
		}
		constexpr sparse_hash_table(const sparse_hash_table &other, const allocator_type &alloc)
			: value_ebo_base(alloc),
			  bucket_ebo_base(bucket_allocator_type{alloc}),
			  compare_ebo_base(other.get_comp()),
			  hash_ebo_base(other.get_hash())
		{
			insert(other.begin(), other.end());
		}

		constexpr sparse_hash_table(sparse_hash_table &&other, const allocator_type &alloc) noexcept(
			nothrow_alloc_copy_move_transfer<allocator_type, bucket_allocator_type>)
			: value_ebo_base(alloc),
			  bucket_ebo_base(std::move(other.get_bucket_allocator())),
			  compare_ebo_base(std::move(other.get_comp())),
			  hash_ebo_base(std::move(other.get_hash()))
		{
			if (alloc_eq(get_allocator(), other.get_allocator()))
				take_data(std::move(other));
			else
				move_values(std::move(other));
		}
		constexpr sparse_hash_table(sparse_hash_table &&other) noexcept(nothrow_alloc_move_construct<allocator_type, bucket_allocator_type>)
			: value_ebo_base(std::move(other.get_allocator())),
			  bucket_ebo_base(std::move(other.get_bucket_allocator())),
			  compare_ebo_base(std::move(other.get_comp())),
			  hash_ebo_base(std::move(other.get_hash()))
		{
			take_data(std::move(other));
		}

		constexpr sparse_hash_table &operator=(const sparse_hash_table &other)
		{
			if (this != &other) copy_assign_impl(other);
			return *this;
		}

		constexpr sparse_hash_table &
			operator=(sparse_hash_table &&other) noexcept(nothrow_alloc_move_assign<allocator_type, bucket_allocator_type>)
		{
			move_assign_impl(std::move(other));
			return *this;
		}

		constexpr ~sparse_hash_table()
		{
			clear();
			delete_buckets(m_buckets_data, m_buckets_capacity);
		}

		[[nodiscard]] constexpr iterator begin() noexcept { return iterator_from_bucket(buckets_start()); }
		[[nodiscard]] constexpr iterator end() noexcept { return iterator_from_bucket(buckets_end()); }
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return iterator_from_bucket(buckets_start()); }
		[[nodiscard]] constexpr const_iterator end() const noexcept { return iterator_from_bucket(buckets_end()); }

		[[nodiscard]] constexpr size_type size() const noexcept { return m_load_count; }
		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			return static_cast<size_type>(static_cast<float>(m_buckets_capacity) * max_load_factor);
		}
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			constexpr auto absolute_max = static_cast<size_type>(std::numeric_limits<difference_type>::max());
			const auto alloc_max = static_cast<size_type>(bucket_alloc_traits::max_size(get_allocator()));

			/* Max size cannot exceed max load factor of max capacity. */
			return static_cast<size_type>(static_cast<float>(std::min(absolute_max, alloc_max)) / sizeof(value_type) *
										  max_load_factor);
		}
		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return m_buckets_capacity; }

		[[nodiscard]] constexpr iterator find(const auto &key) noexcept
		{
			return iterator_from_bucket(find_bucket<true>(key));
		}
		[[nodiscard]] constexpr const_iterator find(const auto &key) const noexcept
		{
			return iterator_from_bucket(find_bucket<true>(key));
		}

		constexpr void clear()
		{
			for (auto item = begin(), last = end(); item != last; ++item) erase_bucket_impl(item.m_bucket_ptr);
			destroy_data();
		}

		constexpr void rehash(size_type new_cap)
		{
			/* Adjust the capacity to be at least large enough to fit the current load count. */
			new_cap = std::max(std::max(static_cast<size_type>(static_cast<float>(size()) / max_load_factor), new_cap),
							   initial_capacity);

			/* Quadratic search implementation requires power of 2 capacity, since the c1 and c2 constants are 0.5. */
			new_cap = next_pow_2(new_cap);

			/* Don't do anything if the capacity did not change after the adjustment. */
			if (new_cap != m_buckets_capacity) [[likely]]
				rehash_impl(new_cap);
		}
		constexpr void reserve(size_type n) { rehash(static_cast<size_type>(static_cast<float>(n) / max_load_factor)); }

		template<typename... Args>
		constexpr std::pair<iterator, bool> emplace(Args &&...args)
		{
			maybe_rehash();

			auto new_bucket = make_bucket(std::forward<Args>(args)...);
			auto dest = find_bucket<false>(new_bucket.key(), new_bucket.hash);
			auto inserted = insert_impl(dest, new_bucket);
			return {iterator_from_bucket(dest), inserted};
		}
		template<typename... Args>
		constexpr std::pair<iterator, bool> try_emplace(const auto &key, Args &&...args)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(key);
			auto inserted = try_emplace_impl(
				dest, std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(std::forward<Args>(args)...));
			return {iterator_from_bucket(dest), inserted};
		}
		template<typename... Args>
		constexpr std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(key);
			auto inserted = try_emplace_impl(dest,
											 std::piecewise_construct,
											 std::forward_as_tuple(std::forward<key_type>(key)),
											 std::forward_as_tuple(std::forward<Args>(args)...));
			return {iterator_from_bucket(dest), inserted};
		}

		constexpr std::pair<iterator, bool> insert(const value_type &value)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(key_extract(value));
			auto inserted = insert_impl(dest, make_bucket(value));
			return {iterator_from_bucket(dest), inserted};
		}
		constexpr std::pair<iterator, bool> insert(value_type &&value)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(key_extract(value));
			auto inserted = insert_impl(dest, make_bucket(std::forward<value_type>(value)));
			return {iterator_from_bucket(dest), inserted};
		}
		constexpr std::pair<iterator, bool> try_insert(const value_type &value)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(key_extract(value));
			auto inserted = try_emplace_impl(dest, value);
			return {iterator_from_bucket(dest), inserted};
		}
		constexpr std::pair<iterator, bool> try_insert(value_type &&value)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(key_extract(value));
			auto inserted = try_emplace_impl(dest, std::forward<value_type>(value));
			return {iterator_from_bucket(dest), inserted};
		}

		template<std::forward_iterator Iter>
		constexpr size_type insert(Iter first, Iter last)
		{
			size_type amount = 0;
			for (; first != last; ++first) amount += emplace(*first).second;
			return amount;
		}
		constexpr size_type insert(iterator first, iterator last)
		{
			size_type amount = 0;
			for (; first != last; ++first) amount += insert(*first).second;
			return amount;
		}
		constexpr size_type insert(const_iterator first, const_iterator last)
		{
			size_type amount = 0;
			for (; first != last; ++first) amount += insert(*first).second;
			return amount;
		}

		template<std::forward_iterator Iter>
		constexpr size_type try_insert(Iter first, Iter last)
		{
			size_type amount = 0;
			for (; first != last; ++first) amount += try_insert(*first).second;
			return amount;
		}

		[[nodiscard]] constexpr node_handle extract_node(const_iterator where)
		{
			SEK_ASSERT(where >= begin() && where < end());
			SEK_ASSERT(where.m_bucket_ptr->is_occupied());

			erase_aux(1);
			return node_handle{*where.m_bucket_ptr, get_allocator()};
		}
		constexpr std::pair<iterator, bool> insert_node(node_handle &&handle)
		{
			maybe_rehash();

			auto dest = find_bucket<false>(handle.m_bucket.key(), handle.m_bucket.hash);
			auto inserted = insert_impl(dest, handle.reset());
			return {iterator_from_bucket(dest), inserted};
		}
		constexpr std::pair<iterator, bool> try_insert_node(node_handle &&handle)
		{
			maybe_rehash();

			if (auto dest = find_bucket<false>(handle.m_bucket.key(), handle.m_bucket.hash); dest->is_occupied())
				return {iterator_from_bucket(dest), false};
			else
			{
				insert_aux(dest);
				*dest = handle.reset();
				return {iterator_from_bucket(dest), true};
			}
		}

		constexpr iterator erase(const_iterator where)
		{
			erase_bucket_impl(where.m_bucket_ptr);
			erase_aux(1);
			return iterator_from_bucket(where.m_bucket_ptr);
		}
		constexpr iterator erase(const_iterator first, const_iterator last)
		{
			size_type amount = 0;
			for (; first < last; ++first, ++amount) erase_bucket_impl(first.m_bucket_ptr);
			erase_aux(amount);
			return iterator_from_bucket(first.m_bucket_ptr);
		}

		[[nodiscard]] constexpr auto load_factor() const noexcept
		{
			return static_cast<float>(size()) / static_cast<float>(bucket_count());
		}
		[[nodiscard]] constexpr auto tombstone_factor() const noexcept
		{
			return static_cast<float>(m_tombstone_count) / static_cast<float>(bucket_count());
		}

		[[nodiscard]] constexpr allocator_type &get_allocator() noexcept { return *value_ebo_base::get(); }
		[[nodiscard]] constexpr const allocator_type &get_allocator() const noexcept { return *value_ebo_base::get(); }
		[[nodiscard]] constexpr bucket_allocator_type &get_bucket_allocator() noexcept
		{
			return *bucket_ebo_base::get();
		}
		[[nodiscard]] constexpr const bucket_allocator_type &get_bucket_allocator() const noexcept
		{
			return *bucket_ebo_base::get();
		}

		[[nodiscard]] constexpr const key_equal &get_comp() const noexcept { return *compare_ebo_base::get(); }
		[[nodiscard]] constexpr const hash_type &get_hash() const noexcept { return *hash_ebo_base::get(); }

		constexpr void swap(sparse_hash_table &other) noexcept
		{
			alloc_assert_swap(get_allocator(), other.get_allocator());
			alloc_assert_swap(get_bucket_allocator(), other.get_bucket_allocator());

			swap_data(other);
			alloc_swap(get_allocator(), other.get_allocator());
			alloc_swap(get_bucket_allocator(), other.get_bucket_allocator());
		}

		/** Current max load factor. */
		float max_load_factor = initial_load_factor;
		/** Current min tombstone factor. */
		float max_tombstone_factor = initial_tombstone_factor;

	private:
		constexpr void take_data(sparse_hash_table &&other) noexcept
		{
			m_buckets_data = std::exchange(other.m_buckets_data, nullptr);
			m_buckets_capacity = std::exchange(other.m_buckets_capacity, 0);
			m_load_count = std::exchange(other.m_load_count, 0);
			m_tombstone_count = std::exchange(other.m_tombstone_count, 0);
			m_consider_shrink = std::exchange(other.m_consider_shrink, false);
		}
		constexpr void swap_data(sparse_hash_table &other) noexcept
		{
			compare_ebo_base::swap(other);
			hash_ebo_base::swap(other);

			using std::swap;
			swap(m_buckets_data, other.m_buckets_data);
			swap(m_buckets_capacity, other.m_buckets_capacity);
			swap(m_load_count, other.m_load_count);
			swap(m_tombstone_count, other.m_tombstone_count);
			swap(m_consider_shrink, other.m_consider_shrink);
		}
		constexpr void move_values(sparse_hash_table &&other)
		{
			auto new_size = other.size();
			reserve(new_size);

			for (auto &value : other) emplace(std::move(value));
			other.clear();
		}
		constexpr void move_assign_impl(sparse_hash_table &&other)
		{
			compare_ebo_base::operator=(std::forward<compare_ebo_base>(other));
			hash_ebo_base::operator=(std::forward<hash_ebo_base>(other));

			if ((alloc_traits::propagate_on_container_move_assignment::value ||
				 alloc_eq(get_allocator(), other.get_allocator())) &&
				(bucket_alloc_traits::propagate_on_container_move_assignment::value ||
				 alloc_eq(get_bucket_allocator(), other.get_bucket_allocator())))
			{
				sparse_hash_table tmp{0, key_equal{}, hash_type{}, get_allocator()};
				swap_data(other);
				tmp.swap_data(other);
				alloc_move_assign(get_allocator(), other.get_allocator());
				alloc_move_assign(get_bucket_allocator(), other.get_bucket_allocator());
			}
			else
			{
				clear();
				move_values(std::move(other));
			}
		}
		constexpr void copy_assign_impl(const sparse_hash_table &other)
		{
			compare_ebo_base::operator=(other);
			hash_ebo_base::operator=(other);

			clear();
			if constexpr (alloc_traits::propagate_on_container_copy_assignment::value || !alloc_traits::is_always_equal::value ||
						  bucket_alloc_traits::propagate_on_container_copy_assignment::value ||
						  !bucket_alloc_traits::is_always_equal::value)
			{
				destroy_data();
				alloc_copy_assign(get_allocator(), other.get_allocator());
				alloc_copy_assign(get_bucket_allocator(), other.get_bucket_allocator());
			}

			reserve(other.size());
			insert(other.begin(), other.end());
		}

		constexpr iterator iterator_from_bucket(bucket_type *bucket) noexcept
		{
			return iterator{bucket, buckets_end()};
		}
		constexpr const_iterator iterator_from_bucket(bucket_type *bucket) const noexcept
		{
			return const_iterator{bucket, buckets_end()};
		}
		[[nodiscard]] constexpr bucket_type *buckets_start() const noexcept { return m_buckets_data; }
		[[nodiscard]] constexpr bucket_type *buckets_end() const noexcept
		{
			return m_buckets_data + m_buckets_capacity;
		}

		constexpr bucket_type *allocate_buckets(size_type capacity)
		{
			auto data = get_bucket_allocator().allocate(capacity);
			std::fill_n(data, capacity, bucket_type{});
			return data;
		}
		constexpr void delete_buckets(bucket_type *data, size_type capacity)
		{
			get_bucket_allocator().deallocate(data, capacity);
		}
		constexpr void destroy_data()
		{
			delete_buckets(std::exchange(m_buckets_data, nullptr), std::exchange(m_buckets_capacity, 0));
			m_consider_shrink = false;
			m_load_count = 0;
			m_tombstone_count = 0;
		}

		template<bool RequireOccupied = true>
		[[nodiscard]] constexpr bucket_type *find_bucket(const auto &key) const noexcept
		{
			return find_bucket<RequireOccupied>(key, get_hash()(key));
		}
		template<bool RequireOccupied = true>
		[[nodiscard]] constexpr bucket_type *find_bucket(const auto &key, auto hash) const noexcept
		{
			return find_bucket_impl<RequireOccupied>(m_buckets_data, m_buckets_capacity, key, hash, get_comp());
		}

		constexpr void rehash_impl(size_type new_cap)
		{
			/* Reset tombstones & shrink flag since the new bucket list will have no tombstones. */
			m_tombstone_count = 0;
			m_consider_shrink = false;

			/* Allocate new array, move all current elements to the new array, then destroy the current one. */
			auto new_data = allocate_buckets(new_cap);
			if (m_buckets_data) [[likely]]
			{
				auto src_begin = begin(), src_end = end();
				for (; src_begin != src_end; ++src_begin)
					if (auto *src = src_begin.m_bucket_ptr; src->is_occupied()) [[likely]]
					{
						auto dest = find_bucket_impl<false>(new_data, new_cap, src->key(), src->hash, get_comp());
						*dest = std::move(*src);
					}

				/* It is safe to deallocate the current node array, since all
				 * pointers should be transferred by now. */
				delete_buckets(m_buckets_data, m_buckets_capacity);
			}
			m_buckets_data = new_data;
			m_buckets_capacity = new_cap;
		}
		constexpr void maybe_rehash()
		{
			if (!m_buckets_capacity) [[unlikely]]
				m_buckets_data = allocate_buckets(m_buckets_capacity = initial_capacity);
			else if (load_factor() > max_load_factor)
				rehash_impl(m_buckets_capacity * 2);
			else if (m_consider_shrink && tombstone_factor() > max_tombstone_factor)
				rehash_impl(next_pow_2(static_cast<size_type>(static_cast<float>(size()) / max_load_factor)));
		}

		template<typename... Args>
		constexpr bucket_type make_bucket(Args &&...args)
		{
			auto *value = std::construct_at(get_allocator().allocate(1), std::forward<Args>(args)...);
			auto hash = get_hash()(key_extract(*value));

			return bucket_type{value, hash};
		}
		constexpr void reset_bucket(bucket_type *bucket)
		{
			if (bucket->is_occupied()) [[likely]]
			{
				std::destroy_at(bucket->data);
				get_allocator().deallocate(bucket->data, 1);
			}
		}

		constexpr void insert_aux(bucket_type *dest)
		{
			m_load_count++;
			if (dest->is_tombstone()) [[unlikely]]
				m_tombstone_count--;
		}
		constexpr bool insert_impl(bucket_type *dest_bucket, bucket_type new_bucket)
		{
			SEK_ASSERT(dest_bucket >= buckets_start() && dest_bucket < buckets_end());

			bool inserted_new = !dest_bucket->is_occupied();
			if (inserted_new)
				insert_aux(dest_bucket);
			else
				reset_bucket(dest_bucket);

			*dest_bucket = new_bucket;
			return inserted_new;
		}
		template<typename... Args>
		constexpr bool try_emplace_impl(bucket_type *dest_bucket, Args &&...args)
		{
			SEK_ASSERT(dest_bucket >= buckets_start() && dest_bucket < buckets_end());

			if (dest_bucket->is_occupied())
				return false;
			else
			{
				insert_aux(dest_bucket);
				*dest_bucket = make_bucket(std::forward<Args>(args)...);
				return true;
			}
		}

		constexpr void erase_aux(size_type amount)
		{
			m_load_count -= amount;
			m_tombstone_count += amount;
			m_consider_shrink = true;
		}
		constexpr void erase_bucket_impl(bucket_type *bucket)
		{
			SEK_ASSERT(bucket >= buckets_start() && bucket < buckets_end());
			SEK_ASSERT(bucket->is_occupied());

			reset_bucket(bucket);
			bucket->set_tombstone();
		}

		/** Pointer to the bucket array. */
		bucket_type *m_buckets_data = nullptr;
		/** Total amount of buckets in the node array. */
		size_type m_buckets_capacity = 0;

		/** Total amount of empty buckets. */
		size_type m_load_count = 0;
		/** Total amount of tombstone buckets. */
		size_type m_tombstone_count = 0;
		/** Flag indicating that the table should consider shrinking on read insert. */
		bool m_consider_shrink = false;
	};
}	 // namespace sek::detail