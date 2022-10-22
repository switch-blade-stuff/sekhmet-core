/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "../../access_guard.hpp"
#include "../../delegate.hpp"
#include "type_data.hpp"

namespace sek
{
	/** @brief Structure used to reflect information about a type. */
	template<typename T>
	class type_factory
	{
		friend class type_info;
		friend class type_database;

		using db_handle = typename shared_guard<type_database *>::unique_handle;
		using data_t = detail::type_data;

		template<basic_static_string Name>
		class constant_t : public detail::type_const
		{
		public:
			[[nodiscard]] static constant_t &instance()
			{
				static constant_t value;
				return value;
			}

		public:
			constexpr constant_t() { name = Name; }

			[[nodiscard]] constexpr bool empty() const noexcept { return get == nullptr; }
		};
		template<typename A>
		class attribute_t : public detail::type_attr
		{
		public:
			[[nodiscard]] static attribute_t &instance()
			{
				static attribute_t value;
				return value;
			}

		public:
			constexpr attribute_t() { type = detail::type_handle{type_selector<A>}; }

			[[nodiscard]] constexpr bool empty() const noexcept { return get == nullptr; }

			[[nodiscard]] constexpr auto *data() noexcept { return m_storage.template get<A>(); }
			[[nodiscard]] constexpr auto *data() const noexcept { return m_storage.template get<A>(); }

		private:
			type_storage<A> m_storage;
		};

		type_factory(shared_guard<type_database *> &&db, data_t *data) : m_db(std::move(db).access()), m_data(data) {}

	public:
		type_factory() = delete;
		type_factory(const type_factory &) = delete;
		type_factory &operator=(const type_factory &) = delete;
		type_factory(type_factory &&) = delete;
		type_factory &operator=(type_factory &&) = delete;

		/** Returns the parent `type_info` of this factory. */
		[[nodiscard]] constexpr type_info type() const noexcept;

		// clang-format off
		/** Adds an attribute to the type.
		 * @param args Arguments passed to attribute's constructor.
		 * @return Reference to this type factory.
		 * @note If the attribute constructor takes `type_factory<T>` as it's first argument, injects the factory instance.
		 * @note Only one attribute of type `A` may be added to any given type. Adding multiple attributes of the same
		 * type will have no effect. */
		template<typename A, typename... Args>
		type_factory &attribute(Args &&...args) requires std::constructible_from<A, type_factory &, Args...> || std::constructible_from<A, Args...>
		{
			/* Ignore initialization if the attribute already exists. */
			if (auto &attr = attribute_t<A>::instance(); attr.empty()) [[likely]]
			{
				if constexpr (std::constructible_from<A, type_factory &, Args...>)
					std::construct_at(attr.data(), *this, std::forward<Args>(args)...);
				else
					std::construct_at(attr.data(), std::forward<Args>(args)...);

				attr.get = +[](const detail::type_attr *ptr)
				{
					const auto attr = static_cast<const attribute_t<A> *>(ptr);
					return forward_any(*attr->data());
				};
				attr.destroy = +[](detail::type_attr *ptr)
				{
					const auto attr = static_cast<attribute_t<A> *>(ptr);
					std::destroy_at(attr->data());
				};
				m_data->attributes.insert(attr);
			}
			return *this;
		}
		// clang-format on

		// clang-format off
		/** Adds a compile-time static constant to the type.
		 * @tparam Name Name of the constant.
		 * @tparam Value Value of the constant.
		 * @return Reference to this type factory.
		 * @note Only one constant with name `Name` may be added to any given type. Adding multiple constants with
		 * the same name will have no effect.
		 * @example
		 * @code{cpp}
		 *
		 * enum class my_enum : int
		 * {
		 * 	MY_VALUE_1 = 1,
		 * 	MY_VALUE_2 = 2,
		 * };
		 *
		 * sek:type_info::reflect<my_enum>()
		 * 		.constant<"MY_VALUE_1", my_enum::MY_VALUE_1>()
		 * 		.constant<"MY_VALUE_2", my_enum::MY_VALUE_2>();
		 * @endcode */
		template<basic_static_string Name, auto Value>
		type_factory &constant() requires(!Name.empty())
		{
			/* Ignore initialization if the constant already exists. */
			if (auto &cn = constant_t<Name>::instance(); cn.empty()) [[likely]]
			{
				cn.get = +[]() { return forward_any(Value); };
				m_data->constants.insert(cn);
			}
			return *this;
		}
		/** Adds a compile-time static constant to the type.
		 * @tparam Name Name of the constant.
		 * @tparam Factory Factory functor used to obtain value of the constant.
		 * @return Reference to this type factory.
		 * @note Only one constant with name `Name` may be added to any given type. Adding multiple constants with
		 * the same name will have no effect.
		 * @example
		 * @code{cpp}
		 *
		 * enum class my_enum : int
		 * {
		 * 	MY_VALUE_1 = 1,
		 * 	MY_VALUE_2 = 2,
		 * };
		 * struct get_my_value_1
		 * {
		 *	[[nodiscard]] constexpr my_enum operator() const noexcept { return my_enum::MY_VALUE_1; }
		 * };
		 * struct get_my_value_2
		 * {
		 *	[[nodiscard]] constexpr my_enum operator() const noexcept { return my_enum::MY_VALUE_2; }
		 * };
		 *
		 * sek:type_info::reflect<my_enum>()
		 * 		.constant<"MY_VALUE_1", get_my_value_1>()
		 * 		.constant<"MY_VALUE_2", get_my_value_2>();
		 * @endcode */
		template<basic_static_string Name, typename Factory>
		type_factory &constant() requires(!Name.empty() && std::is_invocable_v<Factory>)
		{
			/* Ignore initialization if the constant already exists. */
			if (auto &cn = constant_t<Name>::instance(); cn.empty()) [[likely]]
			{
				cn.get = +[]() { return forward_any(Factory{}()); };
				m_data->constants.insert(cn);
			}
			return *this;
		}
		// clang-format on

	private:
		db_handle m_db;
		data_t *m_data;
	};
}	 // namespace sek