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
		/** Adds a type conversion to the type.
		 * @tparam U Converted-to type.
		 * @tparam Conv Converter to use. By default, uses `static_cast<U>`.
		 * @return Reference to this type factory.
		 * @note Only one conversion for type `U` may be added to any given type. Adding multiple conversions for the same
		 * type will have no effect. */
		template<typename U, typename Conv = detail::default_conv<T, U>>
		type_factory &conversion() requires std::is_invocable_r_v<U, Conv, const T &>
		{
			/* Ignore initialization if the conversion already exists. */
			if (auto &conv = detail::type_conv::instance<T, U, Conv>(); conv.empty()) [[likely]]
			{
				m_data->conversions.insert(conv);

				/* Overwrite any default casts if needed. */
				if constexpr (std::is_enum_v<T> && std::same_as<U, std::underlying_type_t<T>>)
				{
					for (auto *&conv_ptr = m_data->conversions.front; conv_ptr != nullptr; conv_ptr = conv_ptr->next)
						if (conv_ptr == m_data->enum_cast)
						{
							conv_ptr = conv_ptr->next;
							break;
						}
					m_data->enum_cast = conv;
				}
				if constexpr (std::same_as<U, std::intmax_t>)
				{
					for (auto *&conv_ptr = m_data->conversions.front; conv_ptr != nullptr; conv_ptr = conv_ptr->next)
						if (conv_ptr == m_data->signed_cast)
						{
							conv_ptr = conv_ptr->next;
							break;
						}
					m_data->signed_cast = conv;
				}
				if constexpr (std::same_as<U, std::uintmax_t>)
				{
					for (auto *&conv_ptr = m_data->conversions.front; conv_ptr != nullptr; conv_ptr = conv_ptr->next)
						if (conv_ptr == m_data->unsigned_cast)
						{
							conv_ptr = conv_ptr->next;
							break;
						}
					m_data->unsigned_cast = conv;
				}
				if constexpr (std::same_as<U, long double>)
				{
					for (auto *&conv_ptr = m_data->conversions.front; conv_ptr != nullptr; conv_ptr = conv_ptr->next)
						if (conv_ptr == m_data->floating_cast)
						{
							conv_ptr = conv_ptr->next;
							break;
						}
					m_data->floating_cast = conv;
				}
			}
			return *this;
		}
		// clang-format on

		// clang-format off
		/** Adds a parent relationship to the type.
		 * @tparam P Parent type of `T`.
		 * @return Reference to this type factory.
		 * @note Only one parent relationship of type `P` may be added to any given type. Adding multiple parents of the
		 * same type will have no effect. */
		template<typename P>
		type_factory &parent() requires std::is_base_of_v<P, T>
		{
			/* Ignore initialization if the conversion already exists. */
			if (auto &parent = detail::type_parent::instance<T, P>(); parent.empty()) [[likely]]
				m_data->parents.insert(parent);
			return *this;
		}
		// clang-format on

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

				attr.get = +[](const detail::type_attr *ptr) -> any_ref
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
				cn.type = detail::type_handle{type_selector<std::remove_cvref_t<decltype(Value)>>};
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
		type_factory &constant() requires(!Name.empty() && std::is_invocable_v<Factory> && std::is_empty_v<Factory>)
		{
			/* Ignore initialization if the constant already exists. */
			if (auto &cn = constant_t<Name>::instance(); cn.empty()) [[likely]]
			{
				cn.type = detail::type_handle{type_selector<std::remove_cvref_t<std::invoke_result_t<Factory>>>};
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