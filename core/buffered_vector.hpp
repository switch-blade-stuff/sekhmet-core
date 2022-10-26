/*
 * Created by switchblade on 2022-10-18.
 */

#pragma once

#include <algorithm>
#include <stdexcept>

#include "detail/alloc_util.hpp"
#include "detail/contiguous_iterator.hpp"
#include "detail/ebo_base_helper.hpp"
#include "detail/flagged_integer.hpp"

namespace sek
{
	/** @brief Drop-in replacement for `std::vector` with support for short buffer optimization.
	 *
	 * Buffered vector operates the same way as the standard `std::vector` type, with additional support
	 * for in-place storage via a local "short" buffer. As long as it's capacity does not exceed the
	 * predefined limit, buffered vector will use the short buffer for it's storage. However, when the
	 * required capacity exceeds this limit, the new "long" buffer will be allocated using the provided
	 * allocator. Unless explicitly requested via `shrink_to_fit`, the long buffer would then be used
	 * indefinitely.
	 *
	 * @tparam T Type of objects stored within the vector.
	 * @tparam N Amount of objects of type `T` stored within the local "short" buffer.
	 * @tparam Alloc Allocator used to allocate memory when vector capacity exceeds `N`. */
	template<typename T, std::size_t N, typename Alloc = std::allocator<T>>
	class buffered_vector : ebo_base_helper<detail::rebind_alloc_t<Alloc, T>>
	{
		using alloc_base = ebo_base_helper<detail::rebind_alloc_t<Alloc, T>>;

	public:
		typedef T value_type;
		typedef contiguous_iterator<T, false> iterator;
		typedef contiguous_iterator<T, true> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef typename iterator::pointer pointer;
		typedef typename const_iterator::pointer const_pointer;
		typedef typename iterator::reference reference;
		typedef typename const_iterator::reference const_reference;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

		typedef Alloc allocator_type;

	private:
		using alloc_traits = std::allocator_traits<detail::rebind_alloc_t<Alloc, T>>;
		using local_data = type_storage<T, N>;
		struct heap_data
		{
			size_type capacity = 0;
			T *data = nullptr;
		};

		constexpr static void relocate_backwards_n(T *src, size_type n, T *dst)
		{
			for (size_type i = n; i-- != 0;) relocate(src + i, dst + i);
		}

		constexpr static void relocate_n(T *src, size_type n, T *dst)
		{
			for (size_type i = 0; i < n; ++i) relocate(src + i, dst + i);
		}

		constexpr static void relocate(T *src, T *dst)
		{
			std::construct_at(dst, std::move(*src));
			std::destroy_at(src);
		}

	public:
		constexpr buffered_vector() noexcept(detail::nothrow_alloc_construct<Alloc>) : m_size(), m_local() {}

		constexpr ~buffered_vector()
		{
			if (local())
				std::destroy_n(m_local.get(), size());
			else
			{
				std::destroy_n(m_heap.data, size());
				alloc_traits::deallocate(alloc(), m_heap.data, m_heap.capacity);
			}
		}

		/** Initializes the vector using the provided allocator instance. */
		constexpr explicit buffered_vector(const allocator_type &alloc) noexcept(std::is_nothrow_constructible_v<Alloc, const Alloc &>)
			: alloc_base(alloc)
		{
		}

		/** Initializes the vector filled with `n` default-constructed elements using the provided allocator instance. */
		constexpr buffered_vector(size_type n, const Alloc &alloc = Alloc{}) : buffered_vector(alloc)
		{
			insert(end(), n);
		}

		/** Initializes the vector filled with `n` copies of `value` using the provided allocator instance. */
		constexpr buffered_vector(size_type n, const value_type &value, const Alloc &alloc = Alloc{})
			: buffered_vector(alloc)
		{
			insert(end(), n, value);
		}

		/** Initializes the vector filled with elements from range `[first, last)` using the provided allocator instance.
		 * @param first Iterator to the start of the source range.
		 * @param last Sentinel of `first`.
		 * @param alloc Allocator used to allocate memory of the vector. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr buffered_vector(I first, S last, const Alloc &alloc = Alloc{}) : buffered_vector(alloc)
		{
			insert(end(), first, last);
		}

		/** Initializes the vector filled with elements from an initializer list using the provided allocator instance.
		 * @param il Initializer list containing vector elements.
		 * @param alloc Allocator used to allocate memory of the vector. */
		constexpr buffered_vector(std::initializer_list<T> il, const Alloc &alloc = Alloc{})
			: buffered_vector(il.begin(), il.end(), alloc)
		{
		}

		/** Copy-constructs the vector. Allocator is copied via `select_on_container_copy_construction`. */
		constexpr buffered_vector(const buffered_vector &other) : alloc_base(detail::alloc_copy(other.alloc()))
		{
			insert(end(), other.begin(), other.end());
		}

		/** Copy-constructs the vector using the provided allocator instance. */
		constexpr buffered_vector(const buffered_vector &other, const Alloc &alloc) : buffered_vector(alloc)
		{
			insert(end(), other.begin(), other.end());
		}

		/** Move-constructs the vector. Allocator is move-constructed. */
		constexpr buffered_vector(buffered_vector &&other) : alloc_base(std::move(other.alloc()))
		{
			if (detail::alloc_eq(alloc(), other.alloc()))
				take_data(other);
			else
				move_data(other);
		}

		/** Move-constructs the vector using the provided allocator instance. */
		constexpr buffered_vector(buffered_vector &&other, const Alloc &alloc) : buffered_vector(alloc)
		{
			if (detail::alloc_eq(alloc(), other.alloc()))
				take_data(other);
			else
				move_data(other);
		}

		constexpr buffered_vector &operator=(const buffered_vector &other)
		{
			if (this != &other) [[likely]]
			{
				if constexpr (alloc_traits::propagate_on_container_copy_assignment::value)
				{
					if (!detail::alloc_eq(alloc(), other.alloc()) && !local())
					{
						std::destroy_n(m_heap.data, size());
						alloc_traits::deallocate(alloc(), m_heap.data, m_heap.capacity);
						m_size = {};
					}
					detail::alloc_copy_assign(alloc(), other.alloc());
				}
				const auto new_size = other.size();
				reserve(new_size);

				/* If possible, copy-assign overlap, then copy-construct or destroy the rest. */
				auto src = other.begin(), src_end = other.end();
				auto dst = begin(), dst_end = end();
				if constexpr (std::is_copy_assignable_v<T>)
					for (; dst != dst_end && src != src_end; ++dst, ++src) *dst = *src;
				if (src < src_end)
					std::uninitialized_copy_n(src, src_end, dst);
				else
					std::destroy(dst, dst_end);
				m_size.value(new_size);
			}
			return *this;
		}

		constexpr buffered_vector &operator=(buffered_vector &&other) noexcept(detail::nothrow_alloc_move_assign<Alloc>)
		{
			if constexpr (alloc_traits::propagate_on_container_move_assignment::value)
			{
				if (!local())
				{
					alloc_traits::deallocate(alloc(), m_heap.data, m_heap.capacity);
					m_size = {};
					m_heap = {};
				}
				detail::alloc_move_assign(alloc(), other.alloc());
				take_data(other);
			}
			else if (detail::alloc_eq(alloc(), other.alloc()))
				take_data(other);
			else
				move_data(other);
			return *this;
		}

		/** Replaces elements of the vector with `n` copies of `value`. */
		constexpr void assign(size_type n, const value_type &value)
		{
			clear();
			insert(end(), n, value);
		}

		/** Replaces elements of the vector with contents of a range `[first, last)`.
		 * @param first Iterator to the start of the source range.
		 * @param last Sentinel of `first`. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr void assign(I first, S last)
		{
			clear();
			insert(end(), first, last);
		}

		/** Replaces elements of the vector with contents of an initializer list.
		 * @param il Initializer list containing new contents of the vector. */
		constexpr void assign(std::initializer_list<T> il)
		{
			clear();
			insert(end(), il);
		}

		/** @copydoc assign */
		constexpr buffered_vector &operator=(std::initializer_list<T> il)
		{
			assign(il);
			return *this;
		}

		/** Returns iterator to the first element of the vector. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{data()}; }

		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{data()}; }

		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }

		/** Returns iterator one past the last element of the vector. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{data() + size()}; }

		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{data() + size()}; }

		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		/** Returns reverse iterator to the last element of the vector. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }

		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }

		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }

		/** Returns reverse iterator one past the firt element of the vector. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }

		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }

		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns the current size of the vector. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_size.value(); }

		/** Returns the maximum possible size of the vector. */
		[[nodiscard]] constexpr size_type max_size() const noexcept { return std::numeric_limits<size_type>() / 2; }

		/** Returns the current capacity of the vector. */
		[[nodiscard]] constexpr size_type capacity() const noexcept { return local() ? N : m_heap.capacity; }

		/** Checks if the vector is empty (`size() == 0`). */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Checks if the vector is stored locally (capacity() <= N). */
		[[nodiscard]] constexpr bool local() const noexcept { return !m_size.flag(); }

		/** Returns reference to the first element of the vector. */
		[[nodiscard]] constexpr reference front() noexcept { return *begin(); }

		/** @copydoc front */
		[[nodiscard]] constexpr const_reference front() const noexcept { return *begin(); }

		/** Returns reference to the last element of the vector. */
		[[nodiscard]] constexpr reference back() noexcept { return std::prev(*end()); }

		/** @copydoc back */
		[[nodiscard]] constexpr const_reference back() const noexcept { return std::prev(*end()); }

		/** Returns reference to the element located at offset `i`. */
		[[nodiscard]] constexpr reference operator[](size_type i) noexcept { return data()[i]; }

		/** @copydoc operator[] */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return data()[i]; }

		/** @copydoc operator[]
		 * @throw std::out_of_range If `i` is out of range. */
		[[nodiscard]] constexpr reference at(size_type i)
		{
			check_subscript(i);
			return operator[](i);
		}

		/** @copydoc at */
		[[nodiscard]] constexpr const_reference at(size_type i) const
		{
			check_subscript(i);
			return operator[](i);
		}

		/** Returns pointer to the internal data buffer. */
		[[nodiscard]] constexpr pointer data() noexcept { return local() ? m_local.get() : m_heap.data; }

		/** Returns pointer to the internal data buffer. */
		[[nodiscard]] constexpr const_pointer data() const noexcept { return local() ? m_local.get() : m_heap.data; }

		/** Destroys all elements stored within the vector. */
		constexpr void clear()
		{
			std::destroy_n(data(), size());
			m_size.value(0);
		}

		/** Shrinks the internal buffer to fit it's size. If the size is less than the local buffer limit,
		 * moves the vector data to local buffer. */
		constexpr void shrink_to_fit()
		{
			/* Only consider shrinking if the vector is non-local. */
			if (!local()) [[likely]]
			{
				const auto s = size();
				if (s < N) /* Move to local storage. */
				{
					const auto old_capacity = m_heap.capacity;
					const auto old_data = m_heap.data;

					relocate_n(old_data, s, m_local.get());
					alloc_traits::deallocate(alloc(), old_data, old_capacity);
					m_size.flag(false);
				}
				else if (s < m_heap.capacity)
				{
					const auto new_data = alloc_traits::allocate(alloc(), s);
					const auto old_capacity = m_heap.capacity;
					const auto old_data = m_heap.data;

					relocate_n(old_data, s, new_data);
					alloc_traits::deallocate(alloc(), old_data, old_capacity);
					m_heap.data = new_data;
					m_heap.capacity = s;
				}
			}
		}

		/** Reserves space for `n` elements, potentially re-allocating internal buffer. */
		constexpr void reserve(size_type n)
		{
			const auto l = local();
			const auto s = size();
			if (l && n > N)
			{
				const auto new_data = alloc_traits::allocate(alloc(), n);
				relocate_n(m_local.get(), s, new_data);
				m_heap.data = new_data;
				m_heap.capacity = n;
				m_size.flag(true);
			}
			else if (n > m_heap.capacity)
			{
				const auto new_data = alloc_traits::allocate(alloc(), n);
				const auto old_capacity = m_heap.capacity;
				const auto old_data = m_heap.data;

				relocate_n(m_local.get(), s, new_data);
				alloc_traits::deallocate(alloc(), old_data, old_capacity);
				m_heap.data = new_data;
				m_heap.capacity = n;
			}
		}

		/** Resizes the vector to `n` elements. Any newly-constructed elements are default-constructed.
		 * @throw std::length_error If the requested size exceeds `max_size`. */
		constexpr void resize(size_type n) { resize_impl(n); }

		/** Resizes the vector to `n` elements. Any newly-constructed elements are copied from `value`.
		 * @throw std::length_error If the requested size exceeds `max_size`. */
		constexpr void resize(size_type n, const value_type &value) { resize_impl(n, value); }

		/** Inserts an in-place constructed element at the specified position.
		 * @param where Position within the vector at which to insert the element.
		 * @param args Arguments passed to the element's constructor.
		 * @return Iterator to the inserted element.
		 * @note Follows the exception guarantee of `std::vector`. */
		template<typename... Args>
		constexpr iterator emplace(const_iterator where, Args &&...args)
		{
			return emplace_impl(where, 1, [&](T *ptr) { std::construct_at(ptr, std::forward<Args>(args)...); });
		}

		/** Inserts an in-place constructed element at the end of the vector.
		 * @param args Arguments passed to the element's constructor.
		 * @return Reference to the inserted element.
		 * @note Follows the exception guarantee of `std::vector`. */
		template<typename... Args>
		constexpr reference emplace_back(Args &&...args)
		{
			return *emplace(end(), std::forward<Args>(args)...);
		}

		/** Inserts a copy-constructed element at the specified position.
		 * @param where Position within the vector at which to insert the element.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element.
		 * @note Follows the exception guarantee of `std::vector`. */
		constexpr iterator insert(const_iterator where, const value_type &value) { return emplace(where, value); }

		/** Inserts a move-constructed element at the specified position.
		 * @param where Position within the vector at which to insert the element.
		 * @param value Value to insert.
		 * @return Iterator to the inserted element.
		 * @note Follows the exception guarantee of `std::vector`. */
		constexpr iterator insert(const_iterator where, value_type &&value) { return emplace(where, std::move(value)); }

		/** Inserts a copy-constructed element at the end of the vector.
		 * @param value Value to insert.
		 * @return Reference to the inserted element.
		 * @note Follows the exception guarantee of `std::vector`. */
		constexpr reference push_back(const value_type &value) { return emplace_back(value); }

		/** Inserts a move-constructed element at the end of the vector.
		 * @param value Value to insert.
		 * @return Reference to the inserted element.
		 * @note Follows the exception guarantee of `std::vector`. */
		constexpr reference push_back(value_type &&value) { return emplace_back(std::move(value)); }

		/** Inserts `n` copies of `value` at the specified position.
		 * @param where Position within the vector at which to insert the elements.
		 * @param n Amount of elements to insert.
		 * @param value Value to insert.
		 * @return Iterator to the start of the inserted sequence.
		 * @note Follows the exception guarantee of `std::vector`. */
		constexpr iterator insert(const_iterator where, size_type n, const value_type &value)
		{
			return emplace_impl(where, n, [&](T *ptr) { std::construct_at(ptr, value); });
		}

		/** Inserts elements from range `[first, last)` at the specified position.
		 * @param where Position within the vector at which to insert the elements.
		 * @param first Iterator to the start of the source range.
		 * @param last Sentinel of `first`.
		 * @return Iterator to the start of the inserted sequence.
		 * @note Follows the exception guarantee of `std::vector`. */
		template<std::forward_iterator I, std::sentinel_for<I> S>
		constexpr iterator insert(const_iterator where, I first, S last)
		{
			auto result = begin() + (where - cbegin());
			const auto back_emplace = where == end();
			try
			{
				for (auto dst = result; first < last; ++dst) dst = insert(dst, where * (first++));
			}
			catch (...)
			{
				/* Follow the strong exception guarantee of `std::vector` and rollback the size. */
				if (back_emplace && first == last) m_size.value(m_size.size() - 1);
				throw;
			}
			return result;
		}

		/** @copydoc insert */
		template<std::random_access_iterator I, std::sentinel_for<I> S>
		constexpr iterator insert(const_iterator where, I first, S last)
		{
			// clang-format off
			return emplace_impl(where,static_cast<size_type>(std::distance(first, last)),
								[&](T *ptr) { std::construct_at(ptr, *first++); });
			// clang-format on
		}

		/** Inserts elements from the initializer list at the specified position.
		 * @param where Position within the vector at which to insert the elements.
		 * @param il Initializer list containing elements to insert.
		 * @return Iterator to the start of the inserted sequence.
		 * @note Follows the exception guarantee of `std::vector`. */
		constexpr iterator insert(const_iterator where, std::initializer_list<T> il)
		{
			return insert(where, il.begin(), il.end());
		}

		/** Removes an element at the specified position.
		 * @param where Position of the element to erase.
		 * @return Iterator to the element after the erased one or `end()`. */
		constexpr iterator erase(const_iterator where) { return erase(where, where + 1); }

		/** Removes a range of elements `[first, last)` from the vector.
		 * @param first Iterator to the start of the erased sequence.
		 * @param last Iterator to the end of the erased sequence.
		 * @return Iterator to the element after the erased sequence or `end()`. */
		constexpr iterator erase(const_iterator first, const_iterator last)
		{
			const auto dst = begin() + (first - cbegin());
			const auto amount = static_cast<size_type>(last - first);
			const auto src = dst + static_cast<difference_type>(amount);
			const auto n_after = static_cast<size_type>(end() - src);
			if constexpr (std::is_move_assignable_v<value_type>)
			{
				std::move(src, end(), dst);
				std::destroy_n(dst.get() + n_after, amount);
			}
			else
			{
				std::destroy_n(dst.get(), static_cast<size_type>(amount));
				relocate_n(src.get(), n_after, dst.get());
			}
			m_size.value(m_size.value() - amount);
			return dst;
		}

		/** Removes the last element from the vector. */
		constexpr void pop_back()
		{
			m_size.value(m_size.value() - 1);
			std::destroy(data() + size());
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return allocator_type{alloc()}; }

		[[nodiscard]] constexpr auto operator<=>(const buffered_vector &other) const noexcept
			requires(requires(const_iterator a, const_iterator b) { std::compare_three_way{}(*a, *b); })
		{
			return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
		}

		[[nodiscard]] constexpr bool operator==(const buffered_vector &other) const noexcept
			requires(requires(const_iterator a, const_iterator b) { std::equal_to<>{}(*a, *b); })
		{
			return std::equal(begin(), end(), other.begin(), other.end());
		}

		// clang-format off
        constexpr void swap(buffered_vector &other) noexcept requires std::is_swappable_v<T> {
            alloc_assert_swap(alloc(), other.alloc());
            alloc_swap(alloc(), other.alloc());
            swap_data(other);
        }

        friend constexpr void
        swap(buffered_vector &a, buffered_vector &b) noexcept requires std::is_swappable_v<T> { a.swap(b); }
		// clang-format on

	private:
		constexpr void check_subscript(size_type i) const
		{
			if (i >= size()) [[unlikely]]
				throw std::out_of_range("`buffered_vector` subscript out of range");
		}

		[[nodiscard]] constexpr auto &alloc() noexcept { return *alloc_base::get(); }

		[[nodiscard]] constexpr auto &alloc() const noexcept { return *alloc_base::get(); }

		constexpr void swap_data(buffered_vector &other) noexcept
			requires std::is_swappable_v<T>
		{
			using std::swap;

			const auto other_local = other.local();
			const auto other_size = other.size();
			const auto this_local = local();
			const auto this_size = size();

			if (this_local && other_local)
			{
				size_type rel_n, rel_pos;
				T *src_data, *dst_data;

				if (this_size > other_size)
				{
					dst_data = other.m_local.get();
					src_data = m_local.get();
					rel_n = this_size - other_size;
					rel_pos = other_size;
				}
				else
				{
					src_data = other.m_local.get();
					dst_data = m_local.get();
					rel_n = other_size - this_size;
					rel_pos = this_size;
				}

				/* Move extra elements. */
				relocate_n(src_data + rel_pos, rel_n, dst_data + rel_pos);

				/* Swap the rest. */
				for (size_type i = 0; i < rel_pos; ++i) swap(dst_data[i], src_data[i]);
			}
			else if (this_local != other_local)
			{
				heap_data *src_heap, *dst_heap;
				T *src_local, *dst_local;
				size_type local_size;

				if (this_local)
				{
					local_size = this_size;
					dst_local = other.m_local.get();
					src_local = m_local.get();
					src_heap = &other.m_heap;
					dst_heap = &m_heap;
				}
				else
				{
					local_size = other_size;
					src_local = other.m_local.get();
					dst_local = m_local.get();
					dst_heap = &other.m_heap;
					src_heap = &m_heap;
				}

				/* Save the heap buffer, move local objects & restore the heap buffer. */
				auto tmp_heap = *src_heap;
				relocate_n(src_local, local_size, dst_local);
				*dst_heap = tmp_heap;
			}
			else
			{
				/* Simply swap heap buffers if both are non-local. */
				swap(m_heap, other.m_heap);
			}
			swap(m_size, other.m_size);
		}

		constexpr void take_data(buffered_vector &other) noexcept
		{
			const auto other_local = other.local();
			const auto other_size = other.size();
			const auto this_local = local();
			const auto this_size = size();

			if (this_local && other_local) /* If both are local, move the elements directly. */
			{
				size_type move_n;
				if (this_size < other_size)
				{
					move_n = this_size;
					const auto diff = this_size - other_size;
					std::uninitialized_move_n(other.m_local.get() + move_n, diff, m_local.get() + move_n);
				}
				else
				{
					move_n = other_size;
					const auto diff = this_size - other_size;
					std::destroy_n(m_local.get() + move_n, diff);
				}

				const auto src = other.m_local.get();
				const auto dst = m_local.get();
				std::move(src, src + move_n, dst);
				m_size = other.m_size;
			}
			else if (other_local) /* Move other's local buffer. */
			{
				/* Destroy the old heap buffer. */
				std::destroy_n(m_heap.data, this_size);
				alloc_traits::deallocate(alloc(), m_heap.data, m_heap.capacity);

				/* Move-construct `other_size` elements. */
				const auto src = other.m_local.get();
				const auto dst = m_local.get();
				std::uninitialized_move_n(src, other_size, dst);
				m_size = other.m_size;
			}
			else if (this_local) /* Move other's heap buffer. */
			{
				/* Destroy the old local buffer. */
				std::destroy_n(m_local.get(), this_size);

				/* Take other's heap buffer. */
				m_size = std::exchange(other.m_size, {});
				m_heap = std::exchange(other.m_heap, {});
			}
			else /* Exchange both heap buffers. */
			{
				std::swap(m_size, other.m_size);
				std::swap(m_heap, other.m_heap);
			}
		}

		constexpr void move_data(buffered_vector &other) noexcept
		{
			const auto other_size = other.size();
			const auto other_end = other.end();

			reserve(other_size);
			const auto this_end = end();

			/* Move-assign overlapping elements. */
			auto dst = begin(), src = other.begin();
			for (; dst != this_end && src != other_end; ++dst, ++src) *dst = std::move(*src);

			/* Move-construct or destroy any extra. */
			if (dst < other_end)
				for (; src != other_end; ++dst, ++src) std::construct_at(dst.get(), std::move(*src));
			else
				for (; dst != this_end; ++dst) std::destroy_at(dst.get());
			m_size.value(other_size);
		}

		template<typename F, typename... Args>
		constexpr iterator emplace_impl(const_iterator where, size_type n, F &&factory)
		{
			const auto insert_pos = static_cast<size_type>(where - cbegin());
			const auto old_size = size();
			const auto new_size = old_size + n;
			const auto old_cap = capacity();
			const auto resized = old_cap <= new_size;
			const auto new_cap = std::max(old_cap * 2, new_size);

			const auto src = data();
			const auto dst = resized ? alloc_traits::allocate(alloc(), new_cap) : src;

			/* Move everything that is above `insert_pos` */
			relocate_backwards_n(src + insert_pos, old_size - insert_pos, dst + insert_pos + n);

			try
			{
				for (size_type i = 0; i < n; ++i) factory(dst + insert_pos + i);
			}
			catch (...)
			{
				/* If a new buffer was allocated, deallocate it. */
				if (resized) alloc_traits::deallocate(alloc(), dst, new_cap);
				throw;
			}

			/* If resizing was necessary, move old data. */
			if (resized)
			{
				relocate_n(src, insert_pos, dst);
				if (!local()) alloc_traits::deallocate(alloc(), src, old_cap);
				m_heap.capacity = new_cap;
				m_heap.data = dst;
				m_size.flag(true);
			}

			return begin() + static_cast<difference_type>(insert_pos);
		}

		constexpr iterator make_space(const_iterator where, size_type n)
		{
			const auto cap = capacity();
			const auto old_size = size();
			const auto new_size = old_size + n;
			const auto iter_pos = static_cast<size_type>(where - begin());

			/* Reallocate the buffer if necessary. */
			if (new_size >= cap) [[unlikely]]
				reserve(std::max(new_size, cap * 2));

			/* Relocate elements to make space. */
			const auto src = begin() + static_cast<difference_type>(iter_pos);
			const auto dst = src + static_cast<difference_type>(n);
			relocate_backwards_n(src.get(), old_size - iter_pos, dst.get());
			m_size.value(new_size);
			return src;
		}

		template<typename... Args>
		constexpr void resize_impl(size_type n, Args &&...args)
		{
			if (n >= max_size()) [[unlikely]]
				throw std::length_error("`buffered_vector` size exceeds maximum allowed limit");

			if (const auto old_size = size(); n < old_size)
			{
				std::destroy_n(data() + n, old_size - n);
				m_size.value(n);
			}
			else if (n > old_size)
			{
				reserve(n);

				/* Exception guarantee. */
				auto dst = data(), pos = dst + old_size, end = dst + n;
				for (; pos < end; ++pos) std::construct_at(pos, args...);
				m_size.value(pos - dst);
			}
		}

		flagged_integer_t<size_type> m_size;
		union
		{
			local_data m_local;
			heap_data m_heap;
		};
	};

	/** Erases all elements that compare equal to `value` from the vector. */
	template<typename T, std::size_t N, typename Alloc, typename U>
	constexpr typename buffered_vector<T, N, Alloc>::size_type erase(buffered_vector<T, N, Alloc> &v, const U &value)
	{
		const auto i = std::remove(v.begin(), v.end(), value);
		const auto d = std::distance(i, v.end());
		v.erase(i, v.end());
		return static_cast<typename buffered_vector<T, N, Alloc>::size_type>(d);
	}

	/** Erases all elements that satisfy the predicate `pred` from the vector. */
	template<typename T, std::size_t N, typename Alloc, typename P>
	constexpr typename buffered_vector<T, N, Alloc>::size_type erase_if(buffered_vector<T, N, Alloc> &v, P &&pred)
	{
		const auto i = std::remove_if(v.begin(), v.end(), std::forward<P>(pred));
		const auto d = std::distance(i, v.end());
		v.erase(i, v.end());
		return static_cast<typename buffered_vector<T, N, Alloc>::size_type>(d);
	}
}	 // namespace sek
