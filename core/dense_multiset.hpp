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

		// clang-format off
		constexpr static bool transparent_key = requires
		{
			typename KeyHash::is_transparent;
			typename KeyComp::is_transparent;
		};
		// clang-format on

		/* General operation of a dense multiset is similar to a regular dense hash table, with the exception
		 * that every entry of the sparse vector contains multiple indices into the dense vector (one per every key).
		 * As such, a multiset maintains multiple bucket chains for every entry, which must be updated synchronously. */

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

		struct dense_entry
		{
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

		template<bool IsConst>
		class multiset_iterator
		{
			template<bool>
			friend class multiset_iterator;
			friend class dense_multiset;

			using iter_t = std::conditional_t<IsConst, typename dense_data::const_iterator, typename dense_data::iterator>;
			using ptr_t = std::conditional_t<IsConst, const dense_entry, dense_entry> *;

		public:
			typedef Key value_type;
			typedef std::conditional_t<IsConst, const value_type, value_type> *pointer;
			typedef std::conditional_t<IsConst, const value_type, value_type> &reference;
			typedef typename table_traits::size_type size_type;
			typedef typename table_traits::difference_type difference_type;
			typedef std::random_access_iterator_tag iterator_category;

		private:
			constexpr explicit multiset_iterator(iter_t iter) noexcept : m_ptr(std::to_address(iter)) {}
			constexpr explicit multiset_iterator(ptr_t ptr) noexcept : m_ptr(ptr) {}

		public:
			constexpr multiset_iterator() noexcept = default;
			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr multiset_iterator(const multiset_iterator<OtherConst> &other) noexcept
				: multiset_iterator(other.m_ptr)
			{
			}

			constexpr multiset_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr multiset_iterator &operator++() noexcept
			{
				++m_ptr;
				return *this;
			}
			constexpr multiset_iterator &operator+=(difference_type n) noexcept
			{
				m_ptr += n;
				return *this;
			}
			constexpr multiset_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr multiset_iterator &operator--() noexcept
			{
				--m_ptr;
				return *this;
			}
			constexpr multiset_iterator &operator-=(difference_type n) noexcept
			{
				m_ptr -= n;
				return *this;
			}

			[[nodiscard]] constexpr multiset_iterator operator+(difference_type n) const noexcept
			{
				return multiset_iterator{m_ptr + n};
			}
			[[nodiscard]] constexpr multiset_iterator operator-(difference_type n) const noexcept
			{
				return multiset_iterator{m_ptr - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const multiset_iterator &other) const noexcept
			{
				return m_ptr - other.m_ptr;
			}

			/** Returns pointer to the target element. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{std::addressof(m_ptr->value)}; }
			/** @copydoc value */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }

			/** Returns reference to the element at an offset. */
			[[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return m_ptr[n].value; }
			/** Returns reference to the target element. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const multiset_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const multiset_iterator &) const noexcept = default;

			constexpr void swap(multiset_iterator &other) noexcept { std::swap(m_ptr, other.m_ptr); }
			friend constexpr void swap(multiset_iterator &a, multiset_iterator &b) noexcept { a.swap(b); }

		private:
			ptr_t m_ptr = {};
		};

	public:
		typedef multiset_iterator<false> iterator;
		typedef multiset_iterator<true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	public:
		constexpr dense_multiset() = default;
		constexpr ~dense_multiset() = default;

		/** Constructs a set with the specified allocators.
		 * @param alloc Allocator used to allocate set's value array.
		 * @param bucket_alloc Allocator used to allocate set's bucket array. */
		constexpr explicit dense_multiset(const allocator_type &alloc) : dense_multiset(key_equal{}, hash_type{}, alloc)
		{
		}
		/** Constructs a set with the specified hasher & allocators.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr explicit dense_multiset(const hash_type &key_hash, const allocator_type &alloc = allocator_type{})
			: dense_multiset(key_equal{}, key_hash, alloc)
		{
		}
		/** Constructs a set with the specified comparator, hasher & allocators.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr explicit dense_multiset(const key_equal &key_compare,
										  const hash_type &key_hash = {},
										  const allocator_type &alloc = allocator_type{})
			: dense_multiset{initial_capacity, key_compare, key_hash, alloc}
		{
		}
		/** Constructs a set with the specified minimum capacity.
		 * @param capacity Capacity of the set.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr explicit dense_multiset(size_type capacity,
										  const KeyComp &key_compare = {},
										  const KeyHash &key_hash = {},
										  const allocator_type &alloc = allocator_type{})
			: m_dense_data{dense_alloc{alloc}, key_compare},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(capacity, sparse_entry{npos}, sparse_alloc{alloc}),
							std::forward_as_tuple(key_hash)}
		{
		}

		/** Constructs a set from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		template<std::random_access_iterator Iterator>
		constexpr dense_multiset(Iterator first,
								 Iterator last,
								 const KeyComp &key_compare = {},
								 const KeyHash &key_hash = {},
								 const allocator_type &alloc = allocator_type{})
			: dense_multiset(static_cast<size_type>(std::distance(first, last)), key_compare, key_hash, alloc)
		{
			insert(first, last);
		}
		/** Constructs a set from a sequence of values.
		 * @param first Iterator to the start of the value sequence.
		 * @param first Iterator to the end of the value sequence.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		template<std::forward_iterator Iterator>
		constexpr dense_multiset(Iterator first,
								 Iterator last,
								 const KeyComp &key_compare = {},
								 const KeyHash &key_hash = {},
								 const allocator_type &alloc = allocator_type{})
			: dense_multiset(key_compare, key_hash, alloc)
		{
			insert(first, last);
		}
		/** Constructs a set from an initializer list.
		 * @param il Initializer list containing values.
		 * @param key_compare Key comparator.
		 * @param key_hash Key hasher.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr dense_multiset(std::initializer_list<value_type> il,
								 const KeyComp &key_compare = {},
								 const KeyHash &key_hash = {},
								 const allocator_type &alloc = allocator_type{})
			: dense_multiset(il.begin(), il.end(), key_compare, key_hash, alloc)
		{
		}

		/** Copy-constructs the set. Allocator is copied via `select_on_container_copy_construction`.
		 * @param other Map to copy data and allocators from. */
		constexpr dense_multiset(const dense_multiset &) = default;
		/** Copy-constructs the set.
		 * @param other Map to copy data and bucket allocator from.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr dense_multiset(const dense_multiset &other, const allocator_type &alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(other.value_vector(), dense_alloc{alloc}),
						   std::forward_as_tuple(other.m_dense_data.second())},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(other.bucket_vector(), sparse_alloc{alloc}),
							std::forward_as_tuple(other.m_sparse_data.second())},
			  m_max_load_factor{other.m_max_load_factor}
		{
		}

		/** Move-constructs the set. Allocator is move-constructed.
		 * @param other Map to move elements and bucket allocator from. */
		constexpr dense_multiset(dense_multiset &&) = default;
		/** Move-constructs the set.
		 * @param other Map to move elements and bucket allocator from.
		 * @param alloc Allocator used to allocate set's value array. */
		constexpr dense_multiset(dense_multiset &&other, const Alloc &alloc)
			: m_dense_data{std::piecewise_construct,
						   std::forward_as_tuple(std::move(other.value_vector()), dense_alloc{alloc}),
						   std::forward_as_tuple(std::move(other.m_dense_data.second()))},
			  m_sparse_data{std::piecewise_construct,
							std::forward_as_tuple(std::move(other.bucket_vector()), sparse_alloc{alloc}),
							std::forward_as_tuple(std::move(other.m_sparse_data.second()))},
			  m_max_load_factor{other.m_max_load_factor}
		{
		}

		/** Copy-assigns the set.
		 * @param other Map to copy elements from. */
		constexpr dense_multiset &operator=(const dense_multiset &) = default;
		/** Move-assigns the set.
		 * @param other Map to move elements from. */
		constexpr dense_multiset &operator=(dense_multiset &&) = default;

		/** Returns iterator to the start of the set. */
		[[nodiscard]] constexpr iterator begin() noexcept { iterator{value_vector().begin()}; }
		/** Returns iterator to the end of the set. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{value_vector().end()}; }
		/** Returns const iterator to the start of the set. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{value_vector().begin()}; }
		/** Returns const iterator to the end of the set. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{value_vector().end()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		/** Returns reverse iterator to the end of the set. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** Returns reverse iterator to the start of the set. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** Returns const reverse iterator to the end of the set. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** Returns const reverse iterator to the start of the set. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Locates an element within the set using the `I`th key.
		 * @tparam I Index of the key to use.
		 * @param key Key to search for.
		 * @return Iterator to the element who's `I`th key is equal to `key`, or end iterator. */
		template<size_type I>
		constexpr const_iterator find(const key_type &key) const noexcept
		{
			const auto entry_idx = find_impl<I>(key_hash(key), key);
			return begin() + static_cast<difference_type>(entry_idx);
		}
		/** Checks if the set contains an element with a matching `I`th key.
		 * @tparam I Index of the key to use.
		 * @param key Key to search for. */
		constexpr bool contains(const key_type &key) const noexcept { return find(key) != end(); }
		// clang-format off
		/** @copydoc find
		 * @note This overload participates in overload resolution only
		 * if both key hasher and key comparator are transparent. */
		template<size_type I>
		constexpr const_iterator find(const auto &key) const noexcept requires transparent_key
		{
			const auto entry_idx = find_impl<I>(key_hash(key), key);
			return begin() + static_cast<difference_type>(entry_idx);
		}
		/** @copydoc contains
		 * @note This overload participates in overload resolution only
		 * if both key hasher and key comparator are transparent. */
		constexpr bool contains(const auto &key) const noexcept requires transparent_key
		{
			return find(key) != end();
		}
		// clang-format on

		/** Empties the set's contents. */
		constexpr void clear()
		{
			std::fill_n(bucket_vector().data(), bucket_count(), sparse_entry{npos});
			value_vector().clear();
		}

		/** Re-hashes the set for the specified minimal capacity. */
		constexpr void rehash(size_type capacity)
		{
			using std::max;

			/* Adjust the capacity to be at least large enough to fit the current size. */
			const auto load_cap = static_cast<size_type>(static_cast<float>(size()) / m_max_load_factor);
			capacity = max(max(load_cap, capacity), initial_capacity);

			/* Don't do anything if the capacity did not change after the adjustment. */
			if (capacity != bucket_vector().capacity()) [[likely]]
				rehash_impl(capacity);
		}
		/** Resizes the internal storage to have space for at least n elements. */
		constexpr void reserve(size_type n)
		{
			value_vector().reserve(n);
			rehash(static_cast<size_type>(static_cast<float>(n) / m_max_load_factor));
		}

		/** Constructs a value (of value_type) in-place, replacing any elements with conflicting keys.
		 * @param args Arguments used to construct the value object.
		 * @return Pair where first element is the iterator to the inserted element and second is the number
		 * of conflicting elements replaced (`0` if none were replaced). */
		template<typename... Args>
		constexpr std::pair<iterator, size_type> emplace(Args &&...args)
		{
			return insert_impl(std::forward<Args>(args)...);
		}

		/** Inserts a value (of value_type) into the set, replacing any elements with conflicting keys.
		 * @param value Value to insert.
		 * @return Pair where first element is the iterator to the inserted element and second is the number
		 * of conflicting elements replaced (`0` if none were replaced). */
		constexpr std::pair<iterator, size_type> insert(value_type &&value)
		{
			return insert_impl(std::forward<value_type>(value));
		}
		/** @copydoc insert */
		constexpr std::pair<iterator, size_type> insert(const value_type &value) { return insert_impl(value); }
		/** @copydetails insert
		 * @param hint Hint for where to insert the value.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element.
		 * @note Hint is required for compatibility with STL algorithms and is ignored. */
		constexpr iterator insert([[maybe_unused]] const_iterator hint, value_type &&value)
		{
			return insert(std::forward<value_type>(value)).first;
		}
		/** @copydoc insert */
		constexpr iterator insert([[maybe_unused]] const_iterator hint, const value_type &value)
		{
			return insert(value).first;
		}

		/** Returns current amount of elements in the set. */
		[[nodiscard]] constexpr size_type size() const noexcept { return value_vector().size(); }
		/** Returns current capacity of the set. */
		[[nodiscard]] constexpr size_type capacity() const noexcept
		{
			return static_cast<size_type>(static_cast<float>(bucket_count()) * m_max_load_factor);
		}
		/** Returns maximum possible amount of elements in the set. */
		[[nodiscard]] constexpr size_type max_size() const noexcept
		{
			const auto max_idx = std::min(value_vector().max_size(), npos - 1);
			return static_cast<size_type>(static_cast<float>(max_idx) * m_max_load_factor);
		}
		/** Checks if the set is empty. */
		[[nodiscard]] constexpr size_type empty() const noexcept { return size() == 0; }

		/** Returns current amount of buckets in the set. */
		[[nodiscard]] constexpr size_type bucket_count() const noexcept { return bucket_vector().size(); }
		/** Returns the maximum amount of buckets. */
		[[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return bucket_vector().max_size(); }

		/** Returns current load factor of the set. */
		[[nodiscard]] constexpr auto load_factor() const noexcept
		{
			return static_cast<float>(size()) / static_cast<float>(bucket_count());
		}
		/** Returns current max load factor of the set. */
		[[nodiscard]] constexpr auto max_load_factor() const noexcept { return m_max_load_factor; }
		/** Sets current max load factor of the set. */
		constexpr void max_load_factor(float f) noexcept
		{
			SEK_ASSERT(f > .0f);
			m_max_load_factor = f;
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept
		{
			return allocator_type{value_allocator()};
		}

		[[nodiscard]] constexpr hash_type hash_function() const noexcept { return get_hash(); }
		[[nodiscard]] constexpr key_equal key_eq() const noexcept { return get_comp(); }

		[[nodiscard]] constexpr bool operator==(const dense_multiset &other) const noexcept
			requires(requires(const_iterator a, const_iterator b) { std::equal_to<>{}(*a, *b); })
		{
			return std::is_permutation(begin(), end(), other.begin(), other.end());
		}

		constexpr void swap(dense_multiset &other) noexcept
		{
			using std::swap;
			swap(m_sparse_data, other.m_sparse_data);
			swap(m_dense_data, other.m_dense_data);
			swap(m_max_load_factor, other.m_max_load_factor);
		}
		friend constexpr void swap(dense_multiset &a, dense_multiset &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] constexpr dense_data &value_vector() noexcept { return m_dense_data.first(); }
		[[nodiscard]] constexpr const dense_data &value_vector() const noexcept { return m_dense_data.first(); }
		[[nodiscard]] constexpr sparse_data &bucket_vector() noexcept { return m_sparse_data.first(); }
		[[nodiscard]] constexpr const sparse_data &bucket_vector() const noexcept { return m_sparse_data.first(); }

		[[nodiscard]] constexpr auto value_allocator() const noexcept { return value_vector().get_allocator(); }
		[[nodiscard]] constexpr auto &get_hash() const noexcept { return m_sparse_data.second(); }
		[[nodiscard]] constexpr auto &get_comp() const noexcept { return m_dense_data.second(); }

		[[nodiscard]] constexpr auto key_hash(const auto &k) const { return m_sparse_data.second()(k); }
		[[nodiscard]] constexpr auto key_comp(const auto &a, const auto &b) const
		{
			return m_dense_data.second()(a, b);
		}

		template<size_type I>
		[[nodiscard]] constexpr size_type *get_chain(hash_t h) noexcept
		{
			const auto idx = h % bucket_vector().size();
			return &(bucket_vector()[idx][I]);
		}
		template<size_type I>
		[[nodiscard]] constexpr const size_type *get_chain(hash_t h) const noexcept
		{
			const auto idx = h % bucket_vector().size();
			return &(bucket_vector()[idx][I]);
		}

		template<size_type I>
		[[nodiscard]] constexpr size_type find_impl(hash_t h, const auto &key) const noexcept
		{
			for (auto *idx = get_chain<I>(h); *idx != npos;)
			{
				const auto &entry = value_vector()[*idx];
				const auto &entry_key = entry.template key<I>();
				const auto entry_hash = entry.hash[I];

				if (entry_hash == h && key_comp(key, entry_key))
					return *idx;
				else
					idx = &entry.next[I];
			}
			return value_vector().size();
		}

		template<size_type... Is>
		constexpr void move_entry(std::index_sequence<Is...>, size_type from, size_type to) noexcept
		{
			auto &src = value_vector()[from];
			auto &dst = value_vector()[to];

			/* Update every bucket chain to reflect the modification. */
			const auto move_chain = [&]<size_type I>(index_selector_t<I>)
			{
				/* Find the chain offset pointing to the old position & replace it with the new position. */
				for (auto *chain_idx = get_chain<I>(src.hash[I]); *chain_idx != npos;
					 chain_idx = &(value_vector()[*chain_idx].next[I]))
					if (*chain_idx == from)
					{
						*chain_idx = to;
						break;
					}
			};
			(move_chain(index_selector<Is>), ...);
			dst = std::move(src);
		}

		template<typename... Args>
		[[nodiscard]] constexpr std::pair<iterator, size_type> insert_impl(Args &&...args)
		{
			return insert_impl(std::make_index_sequence<key_size>{}, std::forward<Args>(args)...);
		}
		template<size_type... Is, typename... Args>
		[[nodiscard]] constexpr std::pair<iterator, size_type> insert_impl(std::index_sequence<Is...>, Args &&...args)
		{
			auto insert_pos = value_vector().size(); /* Position could be modified during replacement. */
			auto *entry_ptr = &value_vector().emplace_back(std::forward<Args>(args)...);

			/* Remove & re-insert the entry for every bucket chain. */
			const auto replace_key = [&]<size_type I>(index_selector_t<I>) -> size_type
			{
				const auto &key = entry_ptr->template key<I>();
				const auto hash = entry_ptr->hash[I] = key_hash(key);

				/* If a conflicting entry within the chain is found, swap it with the last entry & pop the stack. */
				auto *chain_idx = get_chain<I>(hash);
				while (*chain_idx != npos)
				{
					const auto conflict_pos = *chain_idx;
					auto conflict_entry = value_vector().data()[conflict_pos];
					if (conflict_entry.hash[I] == hash && key_comp(key, conflict_entry.template key<I>()))
					{
						/* Save the target index pointer and the old next index. */
						const auto old_next = conflict_entry.next[I];
						auto *target_idx = chain_idx;

						/* If the conflicting entry is not at the end, swap it with the last entry. */
						if (const auto end_pos = size() - 1; conflict_pos != end_pos)
						{
							move_entry(std::make_index_sequence<key_size>{}, end_pos, conflict_pos);

							/* If the inserted entry is the same as the swapped-with entry, update the index. */
							if (insert_pos == end_pos) [[unlikely]]
							{
								entry_ptr = &value_vector()[conflict_pos];
								insert_pos = conflict_pos;
							}
						}

						/* Replace the target index with the inserted position. */
						entry_ptr->next[I] = old_next;
						*target_idx = insert_pos;

						/* Pop the erased entry. */
						value_vector().pop_back();
						return 1;
					}
					chain_idx = &(conflict_entry.next[I]);
				}

				/* If there is no conflicting entry for the current bucket chain, insert it at the end. */
				entry_ptr->next[I] = *chain_idx;
				*chain_idx = insert_pos;
				return 0;
			};

			const auto conflicts = (replace_key(index_selector<Is>) + ...);
			return {iterator{entry_ptr}, conflicts};
		}

		constexpr auto erase_impl(size_type pos) { erase_impl(std::make_index_sequence<key_size>{}, pos); }
		template<size_type... Is>
		constexpr auto erase_impl(std::index_sequence<Is...>, size_type pos)
		{
			const auto chain_erase = [&]<size_type I>(index_selector_t<I>)
			{
				auto &entry = value_vector()[pos];
				const auto &key = entry.template key<I>();
				const auto hash = entry.hash[I];
				for (auto *chain_idx = get_chain<I>(hash); *chain_idx != npos;)
				{
					const auto pos = *chain_idx;
					auto entry_ptr = value_vector().data() + static_cast<difference_type>(pos);

					/* Un-link the entry from the chain. */
					if (entry_ptr->hash[I] == hash && key_comp(key, entry_ptr->template key<I>()))
					{
						*chain_idx = entry_ptr->next[I];
						break;
					}
					chain_idx = &(entry_ptr->next[I]);
				}
			};

			/* Remove the entry from every bucket chain. */
			(chain_erase(index_selector<Is>), ...);
			/* If the entry is not at the end, swap it with the last entry. */
			if (const auto end_pos = size() - 1; pos != end_pos)
			{
				move_entry(std::index_sequence<Is...>{}, end_pos, pos);
				value_vector().pop_back();
			}
			return begin() + static_cast<difference_type>(pos);
		}

		constexpr void maybe_rehash()
		{
			if (load_factor() > m_max_load_factor) [[unlikely]]
				rehash(bucket_count() * 2);
		}
		constexpr void rehash_impl(size_type new_cap) { rehash_impl(std::make_index_sequence<key_size>{}, new_cap); }
		template<size_type... Is>
		constexpr void rehash_impl(std::index_sequence<Is...>, size_type new_cap)
		{
			/* Clear & reserve the vector filled with npos. */
			bucket_vector().clear();
			bucket_vector().resize(new_cap, sparse_entry{npos});

			/* Go through each entry & re-insert it. */
			for (size_type i = 0; i < value_vector().size(); ++i)
			{
				auto &entry = value_vector()[i];
				(chain_insert<Is>(entry, i), ...);
			}
		}

		packed_pair<dense_data, key_equal> m_dense_data;
		packed_pair<sparse_data, hash_type> m_sparse_data = {sparse_data(initial_capacity, sparse_entry{npos}), hash_type{}};

		float m_max_load_factor = initial_load_factor;
	};
}	 // namespace sek