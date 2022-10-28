/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "any.hpp"

namespace sek
{
	namespace detail
	{
		class range_type_iterator
		{
			friend struct range_type_data;

		public:
			typedef any value_type;
			typedef any reference;
			typedef std::forward_iterator_tag iterator_category; /* Minimum guaranteed category */
			typedef std::ptrdiff_t difference_type;
			typedef std::size_t size_type;

		private:
			struct vtable_t
			{
				void (*inc)(range_type_iterator &) = nullptr;
				void (*dec)(range_type_iterator &) = nullptr;
				void (*inc_by)(range_type_iterator &, std::ptrdiff_t) = nullptr;
				void (*dec_by)(range_type_iterator &, std::ptrdiff_t) = nullptr;

				range_type_iterator (*add)(const range_type_iterator &, std::ptrdiff_t) = nullptr;
				range_type_iterator (*sub)(const range_type_iterator &, std::ptrdiff_t) = nullptr;
				std::ptrdiff_t (*diff)(const range_type_iterator &, const range_type_iterator &) = nullptr;

				any (*deref)(const range_type_iterator &) = nullptr;
				any (*deref_at)(const range_type_iterator &, std::ptrdiff_t) = nullptr;

				bool (*cmp_eq)(const range_type_iterator &, const range_type_iterator &) = nullptr;
				bool (*cmp_lt)(const range_type_iterator &, const range_type_iterator &) = nullptr;
				bool (*cmp_le)(const range_type_iterator &, const range_type_iterator &) = nullptr;
				bool (*cmp_gt)(const range_type_iterator &, const range_type_iterator &) = nullptr;
				bool (*cmp_ge)(const range_type_iterator &, const range_type_iterator &) = nullptr;
			};

			template<typename T>
			[[nodiscard]] constexpr static vtable_t make_vtable() noexcept;

			template<typename T>
			static void inc(range_type_iterator &iter)
			{
				iter.template cast<T>()++;
			}
			template<typename T>
			static void dec(range_type_iterator &iter)
			{
				iter.template cast<T>()--;
			}
			template<typename T>
			static void inc(range_type_iterator &iter, difference_type n)
			{
				iter.template cast<T>() += static_cast<std::iter_difference_t<T>>(n);
			}
			template<typename T>
			static void dec(range_type_iterator &iter, difference_type n)
			{
				iter.template cast<T>() -= static_cast<std::iter_difference_t<T>>(n);
			}

			template<typename T>
			static range_type_iterator add(const range_type_iterator &iter, difference_type n)
			{
				return {std::in_place, iter.template cast<T>() + static_cast<std::iter_difference_t<T>>(n)};
			}
			template<typename T>
			static range_type_iterator sub(const range_type_iterator &iter, difference_type n)
			{
				return {std::in_place, iter.template cast<T>() - static_cast<std::iter_difference_t<T>>(n)};
			}
			template<typename T>
			static difference_type diff(const range_type_iterator &a, const range_type_iterator &b)
			{
				return static_cast<difference_type>(a.template cast<T>() - b.template cast<T>());
			}

			template<typename T>
			static any deref(const range_type_iterator &iter)
			{
				return forward_any(*iter.template cast<T>());
			}
			template<typename T>
			static any deref(const range_type_iterator &iter, difference_type i)
			{
				return forward_any(iter.template cast<T>()[static_cast<std::iter_difference_t<T>>(i)]);
			}

			template<typename T>
			static bool cmp_eq(const range_type_iterator &a, const range_type_iterator &b)
			{
				return a == b;
			}
			template<typename T>
			static bool cmp_lt(const range_type_iterator &a, const range_type_iterator &b)
			{
				return a < b;
			}
			template<typename T>
			static bool cmp_le(const range_type_iterator &a, const range_type_iterator &b)
			{
				return a <= b;
			}
			template<typename T>
			static bool cmp_gt(const range_type_iterator &a, const range_type_iterator &b)
			{
				return a > b;
			}
			template<typename T>
			static bool cmp_ge(const range_type_iterator &a, const range_type_iterator &b)
			{
				return a >= b;
			}

			template<typename T>
			static const vtable_t vtable;

			// clang-format off
			template<typename T>
			constexpr range_type_iterator(std::in_place_t, T &&iter) : m_vtable(&vtable<std::decay<T>>), m_data(std::forward<T>(iter))
			{
			}
			// clang-format on

		public:
			constexpr range_type_iterator() noexcept = default;

			/** Returns `true` if the underlying iterator satisfies `std::bidirectional_iterator`. */
			[[nodiscard]] constexpr bool is_bidirectional() const noexcept { return m_vtable->dec; }
			/** Returns `true` if the underlying iterator satisfies `std::random_access_iterator`. */
			[[nodiscard]] constexpr bool is_random_access() const noexcept { return m_vtable->inc_by; }

			range_type_iterator operator++(int)
			{
				auto temp = *this;
				operator++();
				return temp;
			}
			range_type_iterator &operator++()
			{
				m_vtable->inc(*this);
				return *this;
			}
			range_type_iterator &operator+=(difference_type n)
			{
				if (m_vtable->inc_by != nullptr) m_vtable->inc_by(*this, n);
				return *this;
			}
			range_type_iterator operator--(int)
			{
				auto temp = *this;
				operator--();
				return temp;
			}
			range_type_iterator &operator--()
			{
				if (m_vtable->dec != nullptr) m_vtable->dec(*this);
				return *this;
			}
			range_type_iterator &operator-=(difference_type n)
			{
				if (m_vtable->dec_by != nullptr) m_vtable->dec_by(*this, n);
				return *this;
			}

			[[nodiscard]] range_type_iterator operator+(difference_type n) const
			{
				return m_vtable->add ? m_vtable->add(*this, n) : *this;
			}
			[[nodiscard]] range_type_iterator operator-(difference_type n) const
			{
				return m_vtable->sub ? m_vtable->sub(*this, n) : *this;
			}
			[[nodiscard]] difference_type operator-(const range_type_iterator &other) const
			{
				return m_vtable->diff ? m_vtable->diff(*this, other) : 0;
			}

			[[nodiscard]] any value() const { return m_vtable->deref(*this); }
			[[nodiscard]] any operator*() const { return value(); }
			[[nodiscard]] any operator[](difference_type n) const
			{
				return m_vtable->deref_at ? m_vtable->deref_at(*this, n) : any{};
			}

			[[nodiscard]] bool operator==(const range_type_iterator &other) const
			{
				return m_vtable->cmp_eq && m_vtable->cmp_eq(*this, other);
			}
			[[nodiscard]] bool operator<(const range_type_iterator &other) const
			{
				return m_vtable->cmp_lt && m_vtable->cmp_lt(*this, other);
			}
			[[nodiscard]] bool operator<=(const range_type_iterator &other) const
			{
				return m_vtable->cmp_le && m_vtable->cmp_le(*this, other);
			}
			[[nodiscard]] bool operator>(const range_type_iterator &other) const
			{
				return m_vtable->cmp_gt && m_vtable->cmp_gt(*this, other);
			}
			[[nodiscard]] bool operator>=(const range_type_iterator &other) const
			{
				return m_vtable->cmp_ge && m_vtable->cmp_ge(*this, other);
			}

		private:
			template<typename T>
			[[nodiscard]] constexpr T &cast() noexcept
			{
				return *static_cast<T *>(m_data.data());
			}
			template<typename T>
			[[nodiscard]] constexpr const T &cast() const noexcept
			{
				return *static_cast<const T *>(m_data.data());
			}

			const vtable_t *m_vtable = nullptr;
			any m_data;
		};

		template<typename T>
		constexpr range_type_iterator::vtable_t range_type_iterator::make_vtable() noexcept
		{
			vtable_t result;
			result.inc = range_type_iterator::inc<T>;
			result.deref = range_type_iterator::deref<T>;

			if constexpr (std::bidirectional_iterator<T>)
			{
				result.dec = range_type_iterator::dec<T>;
				if constexpr (std::random_access_iterator<T>)
				{
					result.inc_by = range_type_iterator::inc<T>;
					result.dec_by = range_type_iterator::dec<T>;
					result.add = range_type_iterator::add<T>;
					result.sub = range_type_iterator::sub<T>;
					result.diff = range_type_iterator::diff<T>;
					result.deref_at = range_type_iterator::deref<T>;
				}
			}

			if constexpr (std::equality_comparable_with<T, T>) result.cmp_eq = range_type_iterator::cmp_eq<T>;
			if constexpr (std::three_way_comparable_with<T, T>)
			{
				result.cmp_lt = range_type_iterator::cmp_lt<T>;
				result.cmp_le = range_type_iterator::cmp_le<T>;
				result.cmp_gt = range_type_iterator::cmp_gt<T>;
				result.cmp_ge = range_type_iterator::cmp_ge<T>;
			}
			return result;
		};
		template<typename T>
		constexpr range_type_iterator::vtable_t range_type_iterator::vtable = make_vtable<T>();

		template<typename T>
		constexpr range_type_data range_type_data::make_instance() noexcept
		{
			range_type_data result;
			result.value_type = type_handle{type_selector<std::ranges::range_value_t<T>>};

			result.empty = +[](const void *p) -> bool { return std::ranges::empty(*static_cast<const T *>(p)); };
			if constexpr (std::ranges::sized_range<T>)
				result.size = +[](const void *p) -> std::size_t
				{
					const auto &obj = *static_cast<const T *>(p);
					return static_cast<std::size_t>(std::ranges::size(obj));
				};

			result.begin = +[](const any &target) -> range_type_iterator
			{
				if (!target.is_const())
				{
					auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
					return range_type_iterator(std::in_place, std::ranges::begin(obj));
				}
				else
				{
					auto &obj = *static_cast<const T *>(target.data());
					return range_type_iterator(std::in_place, std::ranges::begin(obj));
				}
			};
			result.end = +[](const any &target) -> range_type_iterator
			{
				if (!target.is_const())
				{
					auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
					return range_type_iterator(std::in_place, std::ranges::end(obj));
				}
				else
				{
					auto &obj = *static_cast<const T *>(target.data());
					return range_type_iterator(std::in_place, std::ranges::end(obj));
				}
			};
			result.front = +[](const any &target) -> any
			{
				if (!target.is_const())
				{
					auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
					return forward_any(*std::ranges::begin(obj));
				}
				else
				{
					auto &obj = *static_cast<const T *>(target.data());
					return forward_any(*std::ranges::begin(obj));
				}
			};

			if constexpr (std::ranges::bidirectional_range<T>)
			{
				result.rbegin = +[](const any &target) -> std::reverse_iterator<range_type_iterator>
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return std::reverse_iterator{range_type_iterator(std::in_place, std::ranges::end(obj))};
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::reverse_iterator{range_type_iterator(std::in_place, std::ranges::end(obj))};
					}
				};
				result.rend = +[](const any &target) -> std::reverse_iterator<range_type_iterator>
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return std::reverse_iterator{range_type_iterator(std::in_place, std::ranges::begin(obj))};
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::reverse_iterator{range_type_iterator(std::in_place, std::ranges::begin(obj))};
					}
				};
				result.back = +[](const any &target) -> any
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return forward_any(*std::prev(std::ranges::end(obj)));
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return forward_any(*std::prev(std::ranges::end(obj)));
					}
				};
			}
			if constexpr (std::ranges::random_access_range<T>)
			{
				using diff_type = std::ranges::range_difference_t<T>;
				result.at = +[](const any &target, std::size_t i) -> any
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<const T *>(const_cast<void *>(target.data()));
						if (i < std::ranges::size(obj)) [[likely]]
							return forward_any(std::ranges::begin(obj)[static_cast<diff_type>(i)]);
					}
					else
					{
						auto &obj = *static_cast<T *>(target.data());
						if (i < std::ranges::size(obj)) [[likely]]
							return forward_any(std::ranges::begin(obj)[static_cast<diff_type>(i)]);
					}
					throw std::out_of_range("Range subscript is out of bounds");
				};
			}

			return result;
		}
		template<typename T>
		constinit const range_type_data range_type_data::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Proxy structure used to operate on a range-like type-erased object. */
	class any_range
	{
		friend class any;

		using data_t = detail::range_type_data;

	public:
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		typedef detail::range_type_iterator iterator;
		typedef detail::range_type_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	private:
		static SEK_CORE_PUBLIC const data_t *assert_data(const detail::type_data *data);

		any_range(std::in_place_t, const any &ref) : m_data(ref.m_type->range_data), m_target(ref.ref()) {}
		any_range(std::in_place_t, any &&ref) : m_data(ref.m_type->range_data), m_target(std::move(ref)) {}

	public:
		any_range() = delete;
		any_range(const any_range &) = delete;
		any_range &operator=(const any_range &) = delete;

		constexpr any_range(any_range &&other) noexcept : m_data(other.m_data), m_target(std::move(other.m_target)) {}
		constexpr any_range &operator=(any_range &&other) noexcept
		{
			m_data = other.m_data;
			m_target = std::move(other.m_target);
			return *this;
		}

		/** Initializes an `any_range` instance for an `any` object.
		 * @param ref `any` referencing a range object.
		 * @throw type_error If the referenced object is not a range. */
		explicit any_range(const any &ref) : m_data(assert_data(ref.m_type)), m_target(ref.ref()) {}
		/** @copydoc any_range */
		explicit any_range(any &&ref) : m_data(assert_data(ref.m_type)), m_target(std::move(ref)) {}

		/** Returns `any` reference ot the target range. */
		[[nodiscard]] any target() const noexcept { return m_target.ref(); }

		/** Checks if the referenced range is a sized range. */
		[[nodiscard]] constexpr bool is_sized_range() const noexcept { return m_data->size != nullptr; }
		/** Checks if the referenced range is a bidirectional range. */
		[[nodiscard]] constexpr bool is_bidirectional_range() const noexcept { return m_data->rbegin != nullptr; }
		/** Checks if the referenced range is a random access range. */
		[[nodiscard]] constexpr bool is_random_access_range() const noexcept { return m_data->at != nullptr; }

		/** Returns the value type of the range. */
		[[nodiscard]] constexpr type_info value_type() const noexcept;

		/** Returns iterator to the first element of the referenced range. */
		[[nodiscard]] iterator begin() { return m_data->begin(m_target); }
		/** @copydoc begin */
		[[nodiscard]] const_iterator begin() const { return m_data->begin(m_target); }
		/** @copydoc begin */
		[[nodiscard]] const_iterator cbegin() const { return begin(); }
		/** Returns iterator one past to the last element of the referenced range. */
		[[nodiscard]] iterator end() { return m_data->end(m_target); }
		/** @copydoc end */
		[[nodiscard]] const_iterator end() const { return m_data->end(m_target); }
		/** @copydoc end */
		[[nodiscard]] const_iterator cend() const { return end(); }
		/** Returns reverse iterator to the last element of the referenced range,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rbegin()
		{
			return m_data->rbegin ? m_data->rbegin(m_target) : reverse_iterator{};
		}
		/** @copydoc rbegin */
		[[nodiscard]] const_reverse_iterator rbegin() const
		{
			return m_data->rbegin ? m_data->rbegin(m_target) : const_reverse_iterator{};
		}
		/** @copydoc rbegin */
		[[nodiscard]] const_reverse_iterator crbegin() const { return rbegin(); }
		/** Returns reverse iterator one past the first element of the referenced range,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rend() { return m_data->rend ? m_data->rend(m_target) : reverse_iterator{}; }
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator rend() const
		{
			return m_data->rend ? m_data->rend(m_target) : const_reverse_iterator{};
		}
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator crend() const { return rend(); }

		/** Checks if the referenced range is empty. */
		[[nodiscard]] bool empty() const { return m_data->empty(m_target.data()); }
		/** Returns size of the referenced range, or `0` if the range is not a sized range. */
		[[nodiscard]] size_type size() const { return m_data->size ? m_data->size(m_target.data()) : 0; }

		/** Returns the first object of the referenced range. */
		[[nodiscard]] any front() { return m_data->front(m_target); }
		/** @copydoc front */
		[[nodiscard]] any front() const { return m_data->front(m_target); }
		/** Returns the last object of the referenced range, or empty `any` if the range is not a bidirectional range. */
		[[nodiscard]] any back() { return m_data->back ? m_data->back(m_target) : any{}; }
		/** @copydoc back */
		[[nodiscard]] any back() const { return m_data->back ? m_data->back(m_target) : any{}; }
		/** Returns the `i`th object of the referenced range, or empty `any` if the range is not a random access range.
		 * @throw std::out_of_range If `i` is out of bounds of the range. */
		[[nodiscard]] any at(size_type i) { return m_data->at ? m_data->at(m_target, i) : any{}; }
		/** @copydoc at */
		[[nodiscard]] any operator[](size_type i) { return at(i); }
		/** @copydoc at */
		[[nodiscard]] any at(size_type i) const { return m_data->at ? m_data->at(m_target, i) : any{}; }
		/** @copydoc at */
		[[nodiscard]] any operator[](size_type i) const { return at(i); }

		constexpr void swap(any_range &other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_target, other.m_target);
		}
		friend constexpr void swap(any_range &a, any_range &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data;
		any m_target;
	};
}	 // namespace sek