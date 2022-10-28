/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "any.hpp"

namespace sek
{
	namespace detail
	{
		// clang-format off
		template<typename T>
		concept table_range_type = std::ranges::forward_range<T> && requires
		{
			typename T::key_type;
			typename T::mapped_type;
			pair_like<std::ranges::range_value_t<T>>;
		};
		// clang-format on

		class table_type_iterator
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
				void (*inc)(table_type_iterator &) = nullptr;
				void (*dec)(table_type_iterator &) = nullptr;
				void (*inc_by)(table_type_iterator &, std::ptrdiff_t) = nullptr;
				void (*dec_by)(table_type_iterator &, std::ptrdiff_t) = nullptr;

				table_type_iterator (*add)(const table_type_iterator &, std::ptrdiff_t) = nullptr;
				table_type_iterator (*sub)(const table_type_iterator &, std::ptrdiff_t) = nullptr;
				std::ptrdiff_t (*diff)(const table_type_iterator &, const table_type_iterator &) = nullptr;

				any (*deref_key)(const table_type_iterator &) = nullptr;
				any (*deref_mapped)(const table_type_iterator &) = nullptr;
				std::pair<any, any> (*deref)(const table_type_iterator &) = nullptr;
				std::pair<any, any> (*deref_at)(const table_type_iterator &, std::ptrdiff_t) = nullptr;

				bool (*cmp_eq)(const table_type_iterator &, const table_type_iterator &) = nullptr;
				bool (*cmp_lt)(const table_type_iterator &, const table_type_iterator &) = nullptr;
				bool (*cmp_le)(const table_type_iterator &, const table_type_iterator &) = nullptr;
				bool (*cmp_gt)(const table_type_iterator &, const table_type_iterator &) = nullptr;
				bool (*cmp_ge)(const table_type_iterator &, const table_type_iterator &) = nullptr;
			};

			template<typename T>
			[[nodiscard]] constexpr static vtable_t make_vtable() noexcept;

			template<typename T>
			static void inc(table_type_iterator &iter)
			{
				iter.template cast<T>()++;
			}
			template<typename T>
			static void dec(table_type_iterator &iter)
			{
				iter.template cast<T>()--;
			}
			template<typename T>
			static void inc(table_type_iterator &iter, difference_type n)
			{
				iter.template cast<T>() += static_cast<std::iter_difference_t<T>>(n);
			}
			template<typename T>
			static void dec(table_type_iterator &iter, difference_type n)
			{
				iter.template cast<T>() -= static_cast<std::iter_difference_t<T>>(n);
			}

			template<typename T>
			static table_type_iterator add(const table_type_iterator &iter, difference_type n)
			{
				return {std::in_place, iter.template cast<T>() + static_cast<std::iter_difference_t<T>>(n)};
			}
			template<typename T>
			static table_type_iterator sub(const table_type_iterator &iter, difference_type n)
			{
				return {std::in_place, iter.template cast<T>() - static_cast<std::iter_difference_t<T>>(n)};
			}
			template<typename T>
			static difference_type diff(const table_type_iterator &a, const table_type_iterator &b)
			{
				return static_cast<difference_type>(a.template cast<T>() - b.template cast<T>());
			}

			template<typename T>
			static any deref_key(const table_type_iterator &iter)
			{
				return forward_any(iter.template cast<T>()->first);
			}
			template<typename T>
			static any deref_mapped(const table_type_iterator &iter)
			{
				return forward_any(iter.template cast<T>()->second);
			}
			template<typename T>
			static std::pair<any, any> deref(const table_type_iterator &iter)
			{
				decltype(auto) value = *iter.template cast<T>();
				using first_t = decltype(value.first);
				using second_t = decltype(value.second);
				return {forward_any(std::forward<first_t>(value.first)), forward_any(std::forward<second_t>(value.second))};
			}
			template<typename T>
			static std::pair<any, any> deref(const table_type_iterator &iter, difference_type i)
			{
				decltype(auto) value = iter.template cast<T>()[static_cast<std::iter_difference_t<T>>(i)];
				using first_t = decltype(value.first);
				using second_t = decltype(value.second);
				return {forward_any(std::forward<first_t>(value.first)), forward_any(std::forward<second_t>(value.second))};
			}

			template<typename T>
			static bool cmp_eq(const table_type_iterator &a, const table_type_iterator &b)
			{
				return a == b;
			}
			template<typename T>
			static bool cmp_lt(const table_type_iterator &a, const table_type_iterator &b)
			{
				return a < b;
			}
			template<typename T>
			static bool cmp_le(const table_type_iterator &a, const table_type_iterator &b)
			{
				return a <= b;
			}
			template<typename T>
			static bool cmp_gt(const table_type_iterator &a, const table_type_iterator &b)
			{
				return a > b;
			}
			template<typename T>
			static bool cmp_ge(const table_type_iterator &a, const table_type_iterator &b)
			{
				return a >= b;
			}

			template<typename T>
			static const vtable_t vtable;

			// clang-format off
			template<typename T>
			constexpr table_type_iterator(std::in_place_t, T &&iter) : m_vtable(&vtable<std::decay<T>>), m_data(std::forward<T>(iter))
			{
			}
			// clang-format on

		public:
			constexpr table_type_iterator() noexcept = default;

			/** Returns `true` if the underlying iterator satisfies `std::bidirectional_iterator`. */
			[[nodiscard]] constexpr bool is_bidirectional() const noexcept { return m_vtable->dec; }
			/** Returns `true` if the underlying iterator satisfies `std::random_access_iterator`. */
			[[nodiscard]] constexpr bool is_random_access() const noexcept { return m_vtable->inc_by; }

			table_type_iterator operator++(int)
			{
				auto temp = *this;
				operator++();
				return temp;
			}
			table_type_iterator &operator++()
			{
				m_vtable->inc(*this);
				return *this;
			}
			table_type_iterator &operator+=(difference_type n)
			{
				if (m_vtable->inc_by != nullptr) m_vtable->inc_by(*this, n);
				return *this;
			}
			table_type_iterator operator--(int)
			{
				auto temp = *this;
				operator--();
				return temp;
			}
			table_type_iterator &operator--()
			{
				if (m_vtable->dec != nullptr) m_vtable->dec(*this);
				return *this;
			}
			table_type_iterator &operator-=(difference_type n)
			{
				if (m_vtable->dec_by != nullptr) m_vtable->dec_by(*this, n);
				return *this;
			}

			[[nodiscard]] table_type_iterator operator+(difference_type n) const
			{
				return m_vtable->add ? m_vtable->add(*this, n) : *this;
			}
			[[nodiscard]] table_type_iterator operator-(difference_type n) const
			{
				return m_vtable->sub ? m_vtable->sub(*this, n) : *this;
			}
			[[nodiscard]] difference_type operator-(const table_type_iterator &other) const
			{
				return m_vtable->diff ? m_vtable->diff(*this, other) : 0;
			}

			[[nodiscard]] any key() const { return m_vtable->deref_key(*this); }
			[[nodiscard]] any mapped() const { return m_vtable->deref_mapped(*this); }
			[[nodiscard]] std::pair<any, any> value() const { return m_vtable->deref(*this); }

			[[nodiscard]] std::pair<any, any> operator*() const { value(); }
			[[nodiscard]] std::pair<any, any> operator[](difference_type n) const
			{
				return m_vtable->deref_at ? m_vtable->deref_at(*this, n) : std::pair<any, any>{};
			}

			[[nodiscard]] bool operator==(const table_type_iterator &other) const
			{
				return m_vtable->cmp_eq && m_vtable->cmp_eq(*this, other);
			}
			[[nodiscard]] bool operator<(const table_type_iterator &other) const
			{
				return m_vtable->cmp_lt && m_vtable->cmp_lt(*this, other);
			}
			[[nodiscard]] bool operator<=(const table_type_iterator &other) const
			{
				return m_vtable->cmp_le && m_vtable->cmp_le(*this, other);
			}
			[[nodiscard]] bool operator>(const table_type_iterator &other) const
			{
				return m_vtable->cmp_gt && m_vtable->cmp_gt(*this, other);
			}
			[[nodiscard]] bool operator>=(const table_type_iterator &other) const
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
		constexpr table_type_iterator::vtable_t table_type_iterator::make_vtable() noexcept
		{
			vtable_t result;
			result.inc = table_type_iterator::inc<T>;
			result.deref = table_type_iterator::deref<T>;
			result.deref_key = table_type_iterator::deref_key<T>;
			result.deref_mapped = table_type_iterator::deref_mapped<T>;

			if constexpr (std::bidirectional_iterator<T>)
			{
				result.dec = table_type_iterator::dec<T>;
				if constexpr (std::random_access_iterator<T>)
				{
					result.inc_by = table_type_iterator::inc<T>;
					result.dec_by = table_type_iterator::dec<T>;
					result.add = table_type_iterator::add<T>;
					result.sub = table_type_iterator::sub<T>;
					result.diff = table_type_iterator::diff<T>;
					result.deref_at = table_type_iterator::deref<T>;
				}
			}
			if constexpr (std::equality_comparable_with<T, T>) result.cmp_eq = table_type_iterator::cmp_eq<T>;
			if constexpr (std::three_way_comparable_with<T, T>)
			{
				result.cmp_lt = table_type_iterator::cmp_lt<T>;
				result.cmp_le = table_type_iterator::cmp_le<T>;
				result.cmp_gt = table_type_iterator::cmp_gt<T>;
				result.cmp_ge = table_type_iterator::cmp_ge<T>;
			}
			return result;
		};
		template<typename T>
		constexpr table_type_iterator::vtable_t table_type_iterator::vtable = make_vtable<T>();

		template<typename T>
		constexpr table_type_data table_type_data::make_instance() noexcept
		{
			using key_t = typename T::key_type;
			using mapped_t = typename T::mapped_type;

			// clang-format off
			constexpr bool has_find = requires(const T &t, const key_t &key) { t.find(key); } &&
									  requires(T &t, const key_t &key) { t.find(key); };
			constexpr bool has_at = requires(const T &t, const key_t &key) { t.at(key); } &&
									requires(T &t, const key_t &key) { t.at(key); };
			// clang-format on

			table_type_data result;

			result.value_type = type_handle{type_selector<std::ranges::range_value_t<T>>};
			result.key_type = type_handle{type_selector<key_t>};
			result.mapped_type = type_handle{type_selector<mapped_t>};

			result.empty = +[](const void *p) -> bool { return std::ranges::empty(*static_cast<const T *>(p)); };
			if constexpr (std::ranges::sized_range<T>)
				result.size = +[](const void *p) -> std::size_t
				{
					const auto &obj = *static_cast<const T *>(p);
					return static_cast<std::size_t>(std::ranges::size(obj));
				};

			if constexpr (has_find)
			{
				result.find = +[](const any &target, const any &key) -> table_type_iterator
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return table_type_iterator(std::in_place, obj.find(key_obj));
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return table_type_iterator(std::in_place, obj.find(key_obj));
					}
				};
			}
			if constexpr (requires(const T &t, const key_t &key) { t.contains(key); })
				result.contains = +[](const void *p, const any &key) -> bool
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					return static_cast<const T *>(p)->contains(key_obj);
				};
			else if constexpr (has_find)
				result.contains = +[](const void *p, const any &key) -> bool
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					auto &obj = *static_cast<const T *>(p);
					return obj.find(key_obj) != std::ranges::end(obj);
				};
			if constexpr (has_at)
				result.at = +[](const any &target, const any &key) -> any
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return forward_any(obj.at(key_obj));
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return forward_any(obj.at(key_obj));
					}
					throw std::out_of_range("The specified key is not present within the table");
				};
			else if constexpr (has_find)
				result.at = +[](const any &target, const any &key) -> any
				{
					auto &key_obj = *static_cast<const key_t *>(key.data());
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						if (const auto iter = obj.find(key_obj); iter != std::ranges::end(obj)) [[likely]]
							return forward_any(*iter);
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						if (const auto iter = obj.find(key_obj); iter != std::ranges::end(obj)) [[likely]]
							return forward_any(*iter);
					}
					throw std::out_of_range("The specified key is not present within the table");
				};

			result.begin = +[](const any &target) -> table_type_iterator
			{
				if (!target.is_const())
				{
					auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
					return table_type_iterator(std::in_place, std::ranges::begin(obj));
				}
				else
				{
					auto &obj = *static_cast<const T *>(target.data());
					return table_type_iterator(std::in_place, std::ranges::begin(obj));
				}
			};
			result.end = +[](const any &target) -> table_type_iterator
			{
				if (!target.is_const())
				{
					auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
					return table_type_iterator(std::in_place, std::ranges::end(obj));
				}
				else
				{
					auto &obj = *static_cast<const T *>(target.data());
					return table_type_iterator(std::in_place, std::ranges::end(obj));
				}
			};

			if constexpr (std::ranges::bidirectional_range<T>)
			{
				result.rbegin = +[](const any &target) -> std::reverse_iterator<table_type_iterator>
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return std::reverse_iterator{table_type_iterator(std::in_place, std::ranges::end(obj))};
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::reverse_iterator{table_type_iterator(std::in_place, std::ranges::end(obj))};
					}
				};
				result.rend = +[](const any &target) -> std::reverse_iterator<table_type_iterator>
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return std::reverse_iterator{table_type_iterator(std::in_place, std::ranges::begin(obj))};
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return std::reverse_iterator{table_type_iterator(std::in_place, std::ranges::begin(obj))};
					}
				};
			}

			return result;
		}
		template<typename T>
		constinit const table_type_data table_type_data::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Proxy structure used to operate on a table-like type-erased object. */
	class any_table
	{
		friend class any;

		using data_t = detail::table_type_data;

	public:
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		typedef detail::table_type_iterator iterator;
		typedef detail::table_type_iterator const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	private:
		static SEK_CORE_PUBLIC const data_t *assert_data(const detail::type_data *data);

		any_table(std::in_place_t, const any &ref) : m_data(ref.m_type->table_data), m_target(ref.ref()) {}
		any_table(std::in_place_t, any &&ref) : m_data(ref.m_type->table_data), m_target(std::move(ref)) {}

	public:
		any_table() = delete;
		any_table(const any_table &) = delete;
		any_table &operator=(const any_table &) = delete;

		constexpr any_table(any_table &&other) noexcept : m_data(other.m_data), m_target(std::move(other.m_target)) {}
		constexpr any_table &operator=(any_table &&other) noexcept
		{
			m_data = other.m_data;
			m_target = std::move(other.m_target);
			return *this;
		}

		/** Initializes an `any_table` instance for an `any_ref` object.
		 * @param ref `any_ref` referencing a table object.
		 * @throw type_error If the referenced object is not a table. */
		explicit any_table(const any &ref) : m_data(assert_data(ref.m_type)), m_target(ref.ref()) {}
		/** @copydoc any_table */
		explicit any_table(any &&ref) : m_data(assert_data(ref.m_type)), m_target(std::move(ref)) {}

		/** Returns `any_ref` reference ot the target table. */
		[[nodiscard]] any target() const noexcept { return m_target.ref(); }

		/** Checks if the referenced table is a sized range. */
		[[nodiscard]] constexpr bool is_sized_range() const noexcept { return m_data->size != nullptr; }
		/** Checks if the referenced table is a bidirectional range. */
		[[nodiscard]] constexpr bool is_bidirectional_range() const noexcept { return m_data->rbegin != nullptr; }
		/** Checks if the referenced table is a random access range. */
		[[nodiscard]] constexpr bool is_random_access_range() const noexcept { return m_data->at != nullptr; }

		/** Returns the value type of the table. */
		[[nodiscard]] constexpr type_info value_type() const noexcept;
		/** Returns the key type of the table. */
		[[nodiscard]] constexpr type_info key_type() const noexcept;
		/** Returns the mapped type of the table. */
		[[nodiscard]] constexpr type_info mapped_type() const noexcept;

		/** Returns iterator to the first element of the referenced table. */
		[[nodiscard]] iterator begin() { return m_data->begin(m_target); }
		/** @copydoc begin */
		[[nodiscard]] const_iterator begin() const { return m_data->begin(m_target); }
		/** @copydoc begin */
		[[nodiscard]] const_iterator cbegin() const { return begin(); }
		/** Returns iterator one past to the last element of the referenced table. */
		[[nodiscard]] iterator end() { return m_data->end(m_target); }
		/** @copydoc end */
		[[nodiscard]] const_iterator end() const { return m_data->end(m_target); }
		/** @copydoc end */
		[[nodiscard]] const_iterator cend() const { return end(); }
		/** Returns reverse iterator to the last element of the referenced table,
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
		/** Returns reverse iterator one past the first element of the referenced table,
		 * or a default-constructed sentinel if it is not a bidirectional range. */
		[[nodiscard]] reverse_iterator rend() { return m_data->rend ? m_data->rend(m_target) : reverse_iterator{}; }
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator rend() const
		{
			return m_data->rend ? m_data->rend(m_target) : const_reverse_iterator{};
		}
		/** @copydoc rend */
		[[nodiscard]] const_reverse_iterator crend() const { return rend(); }

		/** Checks if the referenced table contains the specified key. */
		[[nodiscard]] bool contains(const any &key) const
		{
			return m_data->contains && m_data->contains(m_target.data(), key);
		}
		/** Returns iterator to the element of the referenced table located at the specified key, or the end iterator. */
		[[nodiscard]] iterator find(const any &key) { return m_data->find ? m_data->find(m_target, key) : end(); }
		/** @copydoc begin */
		[[nodiscard]] const_iterator find(const any &key) const
		{
			return m_data->find ? m_data->find(m_data, key) : end();
		}

		/** Checks if the referenced table is empty. */
		[[nodiscard]] bool empty() const { return m_data->empty(m_target.data()); }
		/** Returns size of the referenced table, or `0` if the table is not a sized range. */
		[[nodiscard]] size_type size() const { return m_data->size ? m_data->size(m_target.data()) : 0; }

		/** Returns the element of the referenced table located at the specified key.
		 * @throw std::out_of_range If the key is not present within the table. */
		[[nodiscard]] any at(const any &key) { return m_data->at ? m_data->at(m_target, key) : any{}; }
		/** @copydoc at */
		[[nodiscard]] any operator[](const any &key) { return at(key); }
		/** @copydoc at */
		[[nodiscard]] any at(const any &key) const { return m_data->at ? m_data->at(m_target, key) : any{}; }
		/** @copydoc at */
		[[nodiscard]] any operator[](const any &key) const { return at(key); }

		constexpr void swap(any_table &other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_target, other.m_target);
		}
		friend constexpr void swap(any_table &a, any_table &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data;
		any m_target;
	};
}	 // namespace sek