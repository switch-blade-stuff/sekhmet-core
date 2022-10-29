/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "../../access_guard.hpp"
#include "../../delegate.hpp"
#include "type_data.hpp"

namespace sek
{
	namespace detail
	{
		template<typename T, typename... Args>
		attr_data attr_data::make_instance(Args &&...args)
		{
			attr_data result;
			result.template init<T>(std::forward<Args>(args)...);
			result.get_func = +[](const void *data) { return forward_any(*static_cast<const T *>(data)); };
			result.type = type_handle{type_selector<T>};
			return result;
		}

		template<typename T, typename P>
		constexpr base_data base_data::make_instance() noexcept
		{
			base_data result;
			result.cast = +[](const void *ptr) -> const void *
			{
				auto *obj = static_cast<const T *>(ptr);
				return static_cast<const P *>(obj);
			};
			result.type = type_handle{type_selector<P>};
			return result;
		}
		template<typename From, typename To, typename Conv>
		constexpr conv_data conv_data::make_instance() noexcept
		{
			conv_data result;
			result.convert = +[](const any &value) -> any
			{
				if (requires(const From &value) { Conv{}(value); } && !value.is_const())
				{
					auto *obj = static_cast<From *>(const_cast<void *>(value.data()));
					return forward_any(Conv{}(*obj));
				}
				else
				{
					auto *obj = static_cast<const From *>(value.data());
					return forward_any(Conv{}(*obj));
				}
			};
			result.from_type = type_handle{type_selector<From>};
			result.to_type = type_handle{type_selector<To>};
			return result;
		}
		template<typename F>
		constexpr const_data const_data::make_instance(std::string_view name) noexcept
		{
			const_data result;
			result.get = +[]() { return forward_any(F{}()); };
			result.type = type_handle{type_selector<std::remove_cvref_t<std::invoke_result_t<F>>>};
			result.name = name;
			return result;
		}
		template<auto V>
		constexpr const_data const_data::make_instance(std::string_view name) noexcept
		{
			// clang-format off
			struct getter { constexpr decltype(auto) operator()() const noexcept { return V; } };
			return make_instance<getter>(name);
			// clang-format on
		}

		template<typename T, typename U = std::remove_reference_t<T>>
		[[nodiscard]] constexpr static T forward_any_arg(any &value) noexcept
		{
			if constexpr (std::is_const_v<U>)
				return *static_cast<U *>(value.cdata());
			else
				return *static_cast<U *>(value.data());
		}

		template<typename... Args, typename F, std::size_t... Is>
		any ctor_data::invoke_impl(std::index_sequence<Is...>, F &&ctor, std::span<any> args)
		{
			using arg_seq = type_seq_t<Args...>;
			if constexpr (arg_seq::size != 0)
				return std::invoke(ctor, forward_any_arg<pack_element_t<Is, arg_seq>>(args[Is])...);
			else
				return std::invoke(ctor);
		}
		template<typename... Args, typename F>
		any ctor_data::invoke_impl(type_seq_t<Args...>, F &&ctor, std::span<any> args)
		{
			return invoke_impl<Args...>(std::index_sequence<sizeof...(Args)>{}, std::forward<F>(ctor), args);
		}

		template<typename T, typename... Args>
		constexpr ctor_data ctor_data::make_instance(type_seq_t<Args...>) noexcept
		{
			using arg_types = type_seq_t<Args...>;

			ctor_data result;
			result.invoke_func = +[](const void *, std::span<any> args)
			{
				auto ctor = static_cast<any (*)(Args && ...)>(make_any<T, Args...>);
				return invoke_impl(arg_types{}, ctor, args);
			};
			result.args = arg_types_array<arg_types>;
			return result;
		}
		template<typename T, auto F, typename... Args>
		constexpr ctor_data ctor_data::make_instance(type_seq_t<Args...>) noexcept
		{
			using arg_types = type_seq_t<Args...>;

			ctor_data result;
			result.invoke_func = +[](const void *, std::span<any> args)
			{
				// clang-format off
				constexpr auto ctor = [](Args &&...args) { return any::construct_with<T>(F, std::forward<Args>(args)...); };
				return invoke_impl(arg_types{}, ctor, args);
				// clang-format on
			};
			result.args = arg_types_array<arg_types>;
			return result;
		}
		template<typename T, typename F, typename... Args, typename... FArgs>
		ctor_data ctor_data::make_instance(type_seq_t<Args...>, FArgs &&...f_args)
		{
			using arg_types = type_seq_t<Args...>;

			ctor_data result;
			result.template init<F>(std::forward<FArgs>(f_args)...);
			result.invoke_func = +[](const void *data, std::span<any> args)
			{
				const auto ctor = [&](Args &&...args)
				{
					using traits = callable_traits<F>;
					if constexpr (!traits::is_const)
					{
						auto *func = static_cast<F *>(const_cast<void *>(data));
						return any::construct_with<T>(*func, std::forward<Args>(args)...);
					}
					else
					{
						auto *func = static_cast<const F *>(data);
						return any::construct_with<T>(*func, std::forward<Args>(args)...);
					}
				};
				return invoke_impl(arg_types{}, ctor, args);
			};
			result.args = arg_types_array<arg_types>;
			return result;
		}

		template<typename T>
		constexpr dtor_data dtor_data::make_instance() noexcept
		{
			dtor_data result;
			result.invoke_func = +[](const void *, void *ptr)
			{
				auto *obj = static_cast<T *>(ptr);
				std::destroy_at(obj);
			};
			return result;
		}
		template<typename T, auto F>
		constexpr dtor_data dtor_data::make_instance() noexcept
		{
			dtor_data result;
			result.invoke_func = +[](const void *, void *ptr)
			{
				auto *obj = static_cast<T *>(ptr);
				std::invoke(F, obj);
			};
			return result;
		}
		template<typename T, typename F, typename... Args>
		dtor_data dtor_data::make_instance(Args &&...args)
		{
			dtor_data result;
			result.template init<F>(std::forward<Args>(args)...);
			result.invoke_func = +[](const void *data, void *ptr)
			{
				using traits = callable_traits<F>;
				if constexpr (!traits::is_const)
				{
					auto *func = static_cast<F *>(const_cast<void *>(ptr));
					auto *obj = static_cast<T *>(ptr);
					std::invoke(func, obj);
				}
				else
				{
					auto *func = static_cast<const F *>(data);
					auto *obj = static_cast<T *>(ptr);
					std::invoke(func, obj);
				}
			};
			return result;
		}

		template<typename I, typename... Args, typename F, std::size_t... Is>
		constexpr decltype(auto) func_overload::invoke_impl(std::index_sequence<Is...>, F &&func, I *ptr, std::span<any> args)
		{
			using arg_seq = type_seq_t<Args...>;
			if constexpr (arg_seq::size != 0)
				return std::invoke(func, ptr, forward_any_arg<pack_element_t<Is, arg_seq>>(args[Is])...);
			else
				return std::invoke(func, ptr);
		}
		template<typename I, typename... Args, typename F>
		constexpr decltype(auto) func_overload::invoke_impl(type_seq_t<Args...>, F &&func, I *ptr, std::span<any> args)
		{
			return invoke_impl<I, Args...>(std::index_sequence<sizeof...(Args)>{}, std::forward<F>(func), ptr, args);
		}
		template<typename... Args, typename F, std::size_t... Is>
		constexpr decltype(auto) func_overload::invoke_impl(std::index_sequence<Is...>, F &&func, std::span<any> args)
		{
			using arg_seq = type_seq_t<Args...>;
			if constexpr (arg_seq::size != 0)
				return std::invoke(func, forward_any_arg<pack_element_t<Is, arg_seq>>(args[Is])...);
			else
				return std::invoke(func);
		}
		template<typename... Args, typename F>
		constexpr decltype(auto) func_overload::invoke_impl(type_seq_t<Args...>, F &&func, std::span<any> args)
		{
			return invoke_impl<Args...>(std::index_sequence<sizeof...(Args)>{}, std::forward<F>(func), args);
		}

		template<typename T, auto F>
		constexpr func_overload func_overload::make_instance() noexcept
		{
			using traits = func_traits<F>;
			using arg_types = typename traits::arg_types;
			using return_type = typename traits::return_type;

			func_overload result;
			if constexpr (requires { typename traits::instance_type; })
			{
				using instance_type = typename traits::instance_type;
				if constexpr (!std::is_const_v<instance_type>)
					result.invoke_func = +[](const void *, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(const_cast<void *>(ptr));
						return forward_any(invoke_impl(arg_types{}, F, obj, args));
					};
				else
				{
					result.invoke_func = +[](const void *, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(ptr);
						return forward_any(invoke_impl(arg_types{}, F, obj, args));
					};
					result.is_const = true;
				}
				result.instance_type = type_handle{type_selector<instance_type>};
			}
			else
			{
				// clang-format off
				result.invoke_func = +[](const void *, const void *, std::span<any> args)
				{
					return forward_any(invoke_impl(arg_types{}, F, args));
				};
				// clang-format on
			}
			result.args = arg_types_array<arg_types>;
			result.ret = type_handle{type_selector<return_type>};
			return result;
		}
		template<typename T, typename F, typename... FArgs>
		func_overload func_overload::make_instance(FArgs &&...f_args)
		{
			using traits = callable_traits<F>;
			using arg_types = typename traits::arg_types;
			using return_type = typename traits::return_type;

			constexpr auto cast_func = [](const void *ptr)
			{
				if constexpr (!traits::is_const)
					return static_cast<F *>(const_cast<void *>(ptr));
				else
					return static_cast<const F *>(ptr);
			};

			func_overload result;
			result.template init<F>(std::forward<FArgs>(f_args)...);
			if constexpr (requires { typename traits::instance_type; })
			{
				using instance_type = typename traits::instance_type;
				if constexpr (!std::is_const_v<instance_type>)
					result.invoke_func = +[](const void *data, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(const_cast<void *>(ptr));
						auto *func = cast_func(data);
						return forward_any(invoke_impl(arg_types{}, *func, obj, args));
					};
				else
				{
					result.invoke_func = +[](const void *data, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(ptr);
						auto *func = cast_func(data);
						return forward_any(invoke_impl(arg_types{}, *func, obj, args));
					};
					result.is_const = true;
				}
				result.instance_type = type_handle{type_selector<instance_type>};
			}
			else
			{
				// clang-format off
				result.invoke_func = +[](const void *data, const void *, std::span<any> args)
				{
					auto *func = cast_func(data);
					return forward_any(invoke_impl(arg_types{}, *func, args));
				};
				// clang-format on
			}
			result.args = arg_types_array<arg_types>;
			result.ret = type_handle{type_selector<return_type>};
			return result;
		}

		template<typename T>
		type_data type_data::make_instance() noexcept
		{
			type_data result;
			result.reset_func = +[](type_data *data) noexcept { *data = make_instance<T>(); };
			result.name = type_name_v<T>;

			result.is_void = std::is_void_v<T>;
			result.is_empty = std::is_empty_v<T>;
			result.is_nullptr = std::same_as<T, std::nullptr_t>;

			/* For all operations that require an `any`-wrapped object to work, the type must be destructible. */
			if constexpr (std::is_destructible_v<T> && !std::same_as<T, any>)
			{
				/* Add default constructors & destructor. */
				result.destructor = dtor_data::make_instance<T>();
				if constexpr (std::is_default_constructible_v<T>)
					result.constructors.emplace_back(ctor_data::make_instance<T>(type_seq<>));
				if constexpr (std::is_copy_constructible_v<T>)
					result.constructors.emplace_back(ctor_data::make_instance<T>(type_seq<const T &>));

				/* Add default conversions. */
				if constexpr (std::is_enum_v<T>)
				{
					using underlying_t = std::underlying_type_t<T>;
					result.conversions.insert(conv_data::make_instance<T, underlying_t>());
					result.enum_type = type_handle{type_selector<underlying_t>};
				}
				if constexpr (std::signed_integral<T> || std::convertible_to<T, std::intmax_t>)
					result.conversions.insert(conv_data::make_instance<T, std::intmax_t>());
				if constexpr (std::unsigned_integral<T> || std::convertible_to<T, std::uintmax_t>)
					result.conversions.insert(conv_data::make_instance<T, std::uintmax_t>());
				if constexpr (std::floating_point<T> || std::convertible_to<T, long double>)
					result.conversions.insert(conv_data::make_instance<T, long double>());

				result.any_funcs = any_vtable::make_instance<T>();
			}
			return result;
		}
	}	 // namespace detail

	/** @brief Structure used to reflect information about a type. */
	template<typename T>
	class type_factory
	{
		friend class type_info;
		friend class type_database;

		using db_handle = typename shared_guard<type_database *>::unique_handle;

		type_factory(shared_guard<type_database *> &&db, detail::type_data *data)
			: m_db(db.access()), m_target(&data->attributes), m_data(data)
		{
		}

	public:
		type_factory() = delete;
		type_factory(const type_factory &) = delete;
		type_factory &operator=(const type_factory &) = delete;
		type_factory(type_factory &&) = delete;
		type_factory &operator=(type_factory &&) = delete;

		/** Returns the parent `type_info` of this factory. */
		[[nodiscard]] constexpr type_info type() const noexcept;

		/** @brief Adds or overwrites an attribute for the current attribute target (type, constant, function or property).
		 *
		 * Attributes can be added to the underlying type of the factory, as well as the constants,
		 * member functions and member properties of the type. When a constant, function or property
		 * is added to the type, the current attribute target is updated.
		 *
		 * @code{cpp}
		 * type_info::reflect<my_type>()		// Attribute target is set to type data of `my_type`.
		 * 	.attribute<my_attr>()				// Attribute is added to type data `my_type`.
		 * 	.constant<constant_1>("constant_1")	// Attribute target is set to constant "constant_1".
		 * 		.attribute<my_attr>()			// Attribute is added to constant "constant_1".
		 * 	.constant<constant_2>("constant_2")	// Attribute target is set to constant "constant_2".
		 * 		.attribute<my_attr>();			// Attribute is added to constant "constant_2".
		 * @endcode
		 *
		 * @param args Arguments passed to attribute's constructor.
		 * @return Reference to this type factory.
		 * @note If the attribute constructor takes `type_factory<T>` as it's first argument, injects the factory instance. */
		template<typename A, typename... Args>
		type_factory &attribute(Args &&...args)
		{
			if constexpr (std::constructible_from<A, type_factory &, Args...>)
				m_target->insert(detail::attr_data::make_instance<A>(*this, std::forward<Args>(args)...));
			else
				m_target->insert(detail::attr_data::make_instance<A>(std::forward<Args>(args)...));
			return *this;
		}

		// clang-format off
		/** Adds a parent relationship to the type.
		 * @tparam P Parent type of `T`.
		 * @return Reference to this type factory. */
		template<typename P>
		type_factory &parent() requires std::is_base_of_v<P, T>
		{
			m_data->parents.insert(detail::base_data::make_instance<T, P>());
			return *this;
		}
		// clang-format on

		/** Adds a compile-time static constant to the type and changes the attribute target.
		 * @tparam Value Value of the constant.
		 * @param name Name of the constant.
		 * @return Reference to this type factory.
		 *
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
		 * 	.constant<my_enum::MY_VALUE_1>("MY_VALUE_1")
		 * 	.constant<my_enum::MY_VALUE_2>("MY_VALUE_2");
		 * @endcode */
		template<auto Value>
		type_factory &constant(std::string_view name)
		{
			const auto attr = m_data->constants.insert(detail::const_data::make_instance<Value>(name)).first;
			m_target = &attr->attributes;
			return *this;
		}
		/** Adds a compile-time static constant to the type and changes the attribute target.
		 * @tparam Factory Factory functor used to obtain value of the constant.
		 * @param name Name of the constant.
		 * @return Reference to this type factory.
		 *
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
		 *	constexpr my_enum operator() const noexcept { return my_enum::MY_VALUE_1; }
		 * };
		 * struct get_my_value_2
		 * {
		 *	constexpr my_enum operator() const noexcept { return my_enum::MY_VALUE_2; }
		 * };
		 *
		 * sek:type_info::reflect<my_enum>()
		 * 	.constant<get_my_value_1>("MY_VALUE_1")
		 * 	.constant<get_my_value_2>("MY_VALUE_2");
		 * @endcode */
		template<std::invocable Factory>
		type_factory &constant(std::string_view name)
		{
			static_assert(std::is_empty_v<Factory>, "Constant factory functor must be empty");

			const auto attr = m_data->constants.insert(detail::const_data::make_instance<Factory>(name)).first;
			m_target = &attr->attributes;
			return *this;
		}

		/** @brief Adds a static or member function to the type and changes the attribute target.
		 *
		 * If `F` is a free function or a functor that accepts pointer to `T` as the first argument,
		 * it is treated as a member function instead. Additionally, if the first argument is a const
		 * `T` pointer, the function is treated as a constant member function. If a function with the
		 * same name but different arguments already exists, creates an overload.
		 *
		 * @tparam F Free or member function pointer, or a functor instance.
		 * @param name Name of the function.
		 * @return Reference to this type factory.
		 *
		 * @example
		 * @code{cpp}
		 * sek:type_info::reflect<my_type>()
		 * 	.function<&my_type::member_func>("member_func")
		 * 	.function<my_type::static_func>("static_func")
		 * 	.function<functor_type{}>("functor");
		 * @endcode */
		template<auto F>
		type_factory &function(std::string_view name)
		{
			return function_impl(name, detail::func_overload::make_instance<T, F>());
		}
		/** @copybrief function
		 *
		 * If `F` accepts pointer to `T` as the first argument, it is treated as a member function instead.
		 * Additionally, if the first argument is a const `T` pointer, the function is treated as a constant member
		 * function. If a function with the same name but different arguments already exists, creates an overload.
		 *
		 * @tparam F Functor to use as a member function.
		 * @param name Name of the function.
		 * @param args Arguments passed to constructor of `F`.
		 * @return Reference to this type factory.
		 *
		 * @example
		 * @code{cpp}
		 *
		 * struct mem_func
		 * {
		 * 		void operator()(my_type *) const;
		 * };
		 * struct const_mem_func
		 * {
		 * 		void operator()(const my_type *) const;
		 * };
		 * struct static_func
		 * {
		 * 		void operator()(const my_type *) const;
		 * };
		 *
		 * sek:type_info::reflect<my_type>()
		 * 	.function<mem_func>("member_func")			// Mutable overload of `member_func`
		 * 	.function<const_mem_func>("member_func")	// Constant overload of `member_func`
		 * 	.function<static_func>("static_func");
		 * @endcode */
		template<std::invocable F, typename... Args>
		type_factory &function(std::string_view name, Args &&...args)
		{
			static_assert(std::constructible_from<F, Args...>);
			return function_impl(name, detail::func_overload::make_instance<T, F>(std::forward<Args>(args)...));
		}

	private:
		type_factory &function_impl(std::string_view name, detail::func_overload &&overload)
		{
			/* Create the function entry. */
			auto func = m_data->functions.find(name);
			if (func == m_data->functions.end()) [[unlikely]]
				func = m_data->functions.emplace(name).first;

			/* Create or replace the function overload. */
			auto target = std::ranges::find(func->overloads, overload);
			if (target == func->overloads.end()) [[likely]]
				target = func->overloads.emplace(func->overloads.end(), std::move(overload));
			else
				*target = std::move(overload);

			m_target = &target->attributes;
			return *this;
		}

		db_handle m_db;
		detail::attr_table *m_target = nullptr;
		detail::type_data *m_data = nullptr;
	};
}	 // namespace sek