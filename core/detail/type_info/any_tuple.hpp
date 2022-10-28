/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "any.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T>
		template<std::size_t I>
		constexpr void tuple_type_data::getter_t<T>::init() noexcept
		{
			if constexpr (I < size)
			{
				table[I] = +[](T &t) -> any
				{
					using std::get;
					return forward_any(get<I>(t));
				};
				init<I + 1>();
			}
		}
		template<typename T>
		any tuple_type_data::getter_t<T>::operator()(T &t, std::size_t i) const
		{
			if (i < size) [[likely]]
				return table[i](t);
			return any{};
		}
		template<typename T>
		constexpr static tuple_type_data::getter_t<T> tuple_type_getter = tuple_type_data::getter_t<T>{};

		template<typename T>
		constexpr auto tuple_type_data::make_type_array() noexcept
		{
			using array_t = std::array<type_handle, std::tuple_size_v<T>>;
			return []<std::size_t... Is>(std::index_sequence<Is...>)
			{
				return array_t{type_handle{type_selector<pack_element_t<Is, T>>}...};
			}
			(std::make_index_sequence<std::tuple_size_v<T>>{});
		}
		template<typename T>
		constexpr static auto tuple_type_array = tuple_type_data::make_type_array<T>();

		template<typename T>
		constexpr tuple_type_data tuple_type_data::make_instance() noexcept
		{
			tuple_type_data result;
			result.types = std::span{tuple_type_array<T>};

			using std::get;
			if constexpr (requires(T & t) { get<0>(t); })
				result.get = +[](const any &target, std::size_t i) -> any
				{
					if (!target.is_const())
					{
						auto &obj = *static_cast<T *>(const_cast<void *>(target.data()));
						return tuple_type_getter<T>(obj, i);
					}
					else
					{
						auto &obj = *static_cast<const T *>(target.data());
						return tuple_type_getter<const T>(obj, i);
					}
				};

			return result;
		}
		template<typename T>
		constexpr const tuple_type_data tuple_type_data::instance = make_instance<T>();
	}	 // namespace detail

	/** @brief Proxy structure used to operate on a tuple-like type-erased object. */
	class any_tuple
	{
		friend class any;

		using data_t = detail::tuple_type_data;

	public:
		typedef std::size_t size_type;

	private:
		any_tuple(std::in_place_t, const any &ref) : m_data(ref.m_type->tuple_data), m_target(ref.ref()) {}
		any_tuple(std::in_place_t, any &&ref) : m_data(ref.m_type->tuple_data), m_target(std::move(ref)) {}

		static const data_t *assert_data(const detail::type_data *data);

	public:
		any_tuple() = delete;
		any_tuple(const any_tuple &) = delete;
		any_tuple &operator=(const any_tuple &) = delete;

		constexpr any_tuple(any_tuple &&other) noexcept : m_data(other.m_data), m_target(std::move(other.m_target)) {}
		constexpr any_tuple &operator=(any_tuple &&other) noexcept
		{
			m_data = other.m_data;
			m_target = std::move(other.m_target);
			return *this;
		}

		/** Initializes an `any_tuple` instance for an `any_ref` object.
		 * @param ref `any_ref` referencing a table object.
		 * @throw type_error If the referenced object is not a table. */
		explicit any_tuple(const any &ref) : m_data(assert_data(ref.m_type)), m_target(ref.ref()) {}
		/** @copydoc any_tuple */
		explicit any_tuple(any &&ref) : m_data(assert_data(ref.m_type)), m_target(std::move(ref)) {}

		/** Returns `any_ref` reference ot the target tuple. */
		[[nodiscard]] any target() const noexcept { return m_target.ref(); }

		/** Returns the size of the tuple type as if via `std::tuple_size_v<T>`. */
		[[nodiscard]] constexpr size_type size() const noexcept { return m_data->types.size(); }
		/** Returns the `i`th element type of the tuple, or an invalid type info if `i` is out of range. */
		[[nodiscard]] constexpr type_info element(size_type i) const noexcept;

		/** Returns the `i`th element of the referenced tuple, or an empty `any` if `i` is out of range. */
		[[nodiscard]] any get(size_type i) { return m_data->get ? m_data->get(m_target, i) : any{}; }
		/** @copydoc get */
		[[nodiscard]] any get(size_type i) const { return m_data->get ? m_data->get(m_target, i) : any{}; }

		constexpr void swap(any_tuple &other) noexcept
		{
			using std::swap;
			swap(m_data, other.m_data);
			swap(m_target, other.m_target);
		}
		friend constexpr void swap(any_tuple &a, any_tuple &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data;
		any m_target;
	};
}	 // namespace sek