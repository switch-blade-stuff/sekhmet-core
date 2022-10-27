/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include <functional>

#include "any.hpp"
#include "any_range.hpp"
#include "any_string.hpp"
#include "any_table.hpp"
#include "any_tuple.hpp"
#include "type_factory.hpp"

namespace sek
{
	namespace detail
	{
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
			result.convert = +[](any value) -> any
			{
				if (requires(const From &value) { Conv{}(value); } && !value.is_const())
				{
					auto *obj = static_cast<From *>(value.data());
					return forward_any(Conv{}(*obj));
				}
				else
				{
					auto *obj = static_cast<const From *>(value.cdata());
					return forward_any(Conv{}(*obj));
				}
			};
			result.type = type_handle{type_selector<To>};
			return result;
		}
		template<typename T, auto V>
		constexpr const_data const_data::make_instance() noexcept
		{
			const_data result;
			result.get = +[]() { return forward_any(V); };
			result.type = type_handle{type_selector<std::remove_cvref_t<decltype(V)>>};
			return result;
		}

		template<typename T, typename U = std::remove_reference_t<T>>
		[[nodiscard]] constexpr static T forward_any_arg(any &value) noexcept
		{
			if constexpr (std::is_const_v<U>)
				return *static_cast<U *>(value.cdata());
			else
				return *static_cast<U *>(value.data());
		}

		template<typename T, typename... Args, typename F, std::size_t... Is>
		constexpr void ctor_data::invoke_impl(std::index_sequence<Is...>, F &&ctor, T *ptr, std::span<any> args)
		{
			using arg_seq = type_seq_t<Args...>;
			std::invoke(ctor, ptr, forward_any_arg<pack_element_t<Is, arg_seq>>(args[Is])...);
		}
		template<typename T, typename... Args, typename F>
		constexpr void ctor_data::invoke_impl(type_seq_t<Args...>, F &&ctor, T *ptr, std::span<any> args)
		{
			invoke_impl<T, Args...>(std::index_sequence<sizeof...(Args)>{}, std::forward<F>(ctor), ptr, args);
		}

		template<typename T, typename... Args>
		constexpr ctor_data ctor_data::make_instance(type_seq_t<Args...>) noexcept
		{
			using arg_types = type_seq_t<Args...>;

			ctor_data result;
			result.invoke_func = +[](const void *, void *ptr, std::span<any> args)
			{
				constexpr auto ctor = [](T *ptr, Args &&...args) { std::construct_at(ptr, std::forward<Args>(args)...); };
				invoke_impl(arg_types{}, ctor, static_cast<T *>(ptr), args);
			};
			result.args = arg_types_array<arg_types>;
			return result;
		}
		template<typename T, auto F, typename... Args>
		constexpr ctor_data ctor_data::make_instance(type_seq_t<Args...>) noexcept
		{
			using arg_types = type_seq_t<Args...>;

			ctor_data result;
			result.invoke_func = +[](const void *, void *ptr, std::span<any> args)
			{
				auto *obj = static_cast<T *>(ptr);
				invoke_impl(arg_types{}, F, obj, args);
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
			result.invoke_func = +[](const void *data, void *ptr, std::span<any> args)
			{
				using traits = callable_traits<F>;
				if constexpr (!traits::is_const)
				{
					auto *func = static_cast<F *>(const_cast<void *>(ptr));
					auto *obj = static_cast<T *>(ptr);
					invoke_impl(arg_types{}, *func, obj, args);
				}
				else
				{
					auto *func = static_cast<const F *>(data);
					auto *obj = static_cast<T *>(ptr);
					invoke_impl(arg_types{}, *func, obj, args);
				}
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
		constexpr decltype(auto) func_data::invoke_impl(std::index_sequence<Is...>, F &&func, I *ptr, std::span<any> args)
		{
			using arg_seq = type_seq_t<Args...>;
			return std::invoke(func, ptr, forward_any_arg<pack_element_t<Is, arg_seq>>(args[Is])...);
		}
		template<typename I, typename... Args, typename F>
		constexpr decltype(auto) func_data::invoke_impl(type_seq_t<Args...>, F &&func, I *ptr, std::span<any> args)
		{
			return invoke_impl<I, Args...>(std::index_sequence<sizeof...(Args)>{}, std::forward<F>(func), ptr, args);
		}
		template<typename... Args, typename F, std::size_t... Is>
		constexpr decltype(auto) func_data::invoke_impl(std::index_sequence<Is...>, F &&func, std::span<any> args)
		{
			using arg_seq = type_seq_t<Args...>;
			return std::invoke(func, forward_any_arg<pack_element_t<Is, arg_seq>>(args[Is])...);
		}
		template<typename... Args, typename F>
		constexpr decltype(auto) func_data::invoke_impl(type_seq_t<Args...>, F &&func, std::span<any> args)
		{
			return invoke_impl<Args...>(std::index_sequence<sizeof...(Args)>{}, std::forward<F>(func), args);
		}

		template<typename T, auto F>
		constexpr func_data func_data::make_instance() noexcept
		{
			using traits = func_traits<F>;
			using arg_types = typename traits::arg_types;
			using return_type = typename traits::return_type;

			func_data result;
			if constexpr (requires { typename traits::instance_type; })
			{
				using instance_type = typename traits::instance_type;
				if constexpr (!std::is_const_v<instance_type>)
				{
					result.invoke_func = +[](const void *, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(const_cast<void *>(ptr));
						return forward_any(invoke_impl(arg_types{}, F, obj, args));
					};
					result.is_const = false;
				}
				else
				{
					result.invoke_func = +[](const void *, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(ptr);
						return forward_any(invoke_impl(arg_types{}, F, obj, args));
					};
					result.is_const = true;
				}
				result.is_static = false;
			}
			else
			{
				// clang-format off
				result.invoke_func = +[](const void *, const void *, std::span<any> args)
				{
					return forward_any(invoke_impl(arg_types{}, F, args));
				};
				// clang-format on
				result.is_static = true;
				result.is_const = false;
			}
			result.args = arg_types_array<arg_types>;
			result.ret = type_handle{type_selector<return_type>};
			return result;
		}
		template<typename T, typename F, typename... FArgs>
		func_data func_data::make_instance(FArgs &&...f_args)
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

			func_data result;
			result.template init<F>(std::forward<FArgs>(f_args)...);
			if constexpr (requires { typename traits::instance_type; })
			{
				using instance_type = typename traits::instance_type;
				if constexpr (!std::is_const_v<instance_type>)
				{
					result.invoke_func = +[](const void *data, const void *ptr, std::span<any> args)
					{
						auto *obj = static_cast<instance_type *>(const_cast<void *>(ptr));
						auto *func = cast_func(data);
						return forward_any(invoke_impl(arg_types{}, *func, obj, args));
					};
					result.is_const = false;
				}
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
				result.is_static = false;
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
				result.is_static = true;
				result.is_const = false;
			}
			result.args = arg_types_array<arg_types>;
			result.ret = type_handle{type_selector<return_type>};
			return result;
		}

		template<typename T>
		type_data type_data::make_instance() noexcept
		{
			type_data result;
			result.reset = +[](type_data *data) { *data = make_instance<T>(); };
			result.name = type_name_v<T>;

			result.is_void = std::is_void_v<T>;
			result.is_empty = std::is_empty_v<T>;
			result.is_nullptr = std::same_as<T, std::nullptr_t>;

			/* For all operations that require an `any`-wrapped object to work, the type must be destructible. */
			if constexpr (std::is_destructible_v<T>)
			{
				/* Add default constructors & destructor. */
				result.dtor = dtor_data::make_instance<T>();
				if constexpr (std::is_default_constructible_v<T>)
					result.constructors.emplace_back(ctor_data::make_instance<T>(type_seq<>));
				if constexpr (std::is_copy_constructible_v<T>)
					result.constructors.emplace_back(ctor_data::make_instance<T>(type_seq<const T &>));

				/* Add default conversions. */
				if constexpr (std::is_enum_v<T>)
				{
					using underlying_t = std::underlying_type_t<T>;
					result.conversions.emplace(conv_data::make_instance<T, underlying_t>());
					result.enum_type = type_handle{type_selector<underlying_t>};
				}
				if constexpr (std::signed_integral<T> || std::is_convertible_v<T, std::intmax_t>)
					result.conversions.emplace(conv_data::make_instance<T, std::intmax_t>());
				if constexpr (std::unsigned_integral<T> || std::is_convertible_v<T, std::uintmax_t>)
					result.conversions.emplace(conv_data::make_instance<T, std::uintmax_t>());
				if constexpr (std::floating_point<T> || std::is_convertible_v<T, long double>)
					result.conversions.emplace(conv_data::make_instance<T, long double>());

				result.any_funcs = any_vtable::make_instance<T>();
			}
			return result;
		}
	}	 // namespace detail

	/** @brief Handle to information about a reflected type. */
	class type_info
	{
		friend class any;
		friend class any_range;
		friend class any_table;
		friend class any_tuple;
		friend class any_string;
		template<typename>
		friend class type_factory;
		friend class type_database;

		using data_t = detail::type_data;
		using handle_t = detail::type_handle;

		template<typename T>
		[[nodiscard]] constexpr static handle_t handle() noexcept
		{
			return handle_t{type_selector<std::remove_cvref_t<T>>};
		}

		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

		constexpr explicit type_info(const data_t *data) noexcept : m_data(data) {}
		constexpr explicit type_info(handle_t handle) noexcept : m_data(handle.get ? handle.get() : nullptr) {}

	public:
		/** Returns type info for type `T`.
		 * @note Removes any const & volatile qualifiers and decays references.
		 * @note The returned type info is generated at compile time and may not be present within the type database. */
		template<typename T>
		[[nodiscard]] constexpr static type_info get()
		{
			return type_info{handle<T>()};
		}
		/** Searches for a reflected type in the type database.
		 * @return Type info of the type, or an invalid type info if such type is not found. */
		[[nodiscard]] inline static type_info get(std::string_view name);

		/** Reflects type `T`, making it available for runtime lookup by name.
		 * @return Type factory for type `T`, which can be used to specify additional information about the type.
		 * @throw type_error If the type is already reflected. */
		template<typename T>
		inline static type_factory<T> reflect();
		/** Resets a reflected type, removing it from the type database.
		 * @note The type will no longer be available for runtime lookup. */
		inline static void reset(std::string_view name);
		/** @copydoc reset */
		static void reset(type_info type) { reset(type.name()); }
		/** @copydoc reset */
		template<typename T>
		static void reset() noexcept
		{
			reset(type_name_v<std::remove_cvref_t<T>>);
		}

	public:
		/** Initializes an invalid type info handle. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a valid type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }
		/** If the type info references a valid type, returns it's name. Otherwise, returns an empty string view. */
		[[nodiscard]] constexpr std::string_view name() const noexcept
		{
			return valid() ? m_data->name : std::string_view{};
		}

		/** Checks if the referenced type is `void`. */
		[[nodiscard]] constexpr bool is_void() const noexcept { return valid() && m_data->is_void; }
		/** Checks if the referenced type is empty (as if via `std::is_empty_v`). */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return valid() && m_data->is_empty; }
		/** Checks if the referenced type is `std::nullptr_t`, or can be implicitly converted to `std::nullptr_t`. */
		[[nodiscard]] constexpr bool is_nullptr() const noexcept { return valid() && m_data->is_nullptr; }

		/** Checks if the referenced type is an enum (as if via `std::is_enum_v`). */
		[[nodiscard]] constexpr bool is_enum() const noexcept { return valid() && m_data->enum_type.get; }
		/** Checks if the referenced type is a signed integral type or can be implicitly converted to `std::intptr_t`. */
		[[nodiscard]] constexpr bool is_signed() const noexcept { return valid() && m_data->is_int; }
		/** Checks if the referenced type is an unsigned integral type or can be implicitly converted to `std::uintptr_t`. */
		[[nodiscard]] constexpr bool is_unsigned() const noexcept { return valid() && m_data->is_uint; }
		/** Checks if the referenced type is a floating-point type or can be implicitly converted to `long double`. */
		[[nodiscard]] constexpr bool is_floating() const noexcept { return valid() && m_data->is_float; }

		/** Checks if the referenced type is a range. */
		[[nodiscard]] constexpr bool is_range() const noexcept { return valid() && m_data->range_data; }
		/** Checks if the referenced type is a table (range who's value type is a key-value pair). */
		[[nodiscard]] constexpr bool is_table() const noexcept { return valid() && m_data->table_data; }
		/** Checks if the referenced type is tuple-like. */
		[[nodiscard]] constexpr bool is_tuple() const noexcept { return valid() && m_data->tuple_data; }
		/** Checks if the referenced type is string-like. */
		[[nodiscard]] constexpr bool is_string() const noexcept { return valid() && m_data->string_data; }

		/** Returns the referenced type of an enum (as if via `std::underlying_type_t`), or an invalid type info it the type is not an enum. */
		[[nodiscard]] constexpr type_info enum_type() const noexcept
		{
			return valid() ? type_info{m_data->enum_type} : type_info{};
		}
		/** Returns the size of the tuple, or `0` if the type is not a tuple. */
		[[nodiscard]] constexpr std::size_t tuple_size() const noexcept
		{
			return is_tuple() ? m_data->tuple_data->types.size() : 0;
		}
		/** Returns the `i`th element type of the tuple, or an invalid type info if the type is not a tuple or `i` is out of range. */
		[[nodiscard]] constexpr type_info tuple_element(std::size_t i) const noexcept
		{
			return i < tuple_size() ? type_info{m_data->tuple_data->types[i]} : type_info{};
		}

		/** Checks if the referenced type inherits another. That is, the other type is one of it's direct or inherited parents.
		 * @note Does not check if types are the same. */
		[[nodiscard]] SEK_CORE_PUBLIC bool inherits(type_info type) const noexcept;
		/** @copydoc inherits */
		template<typename T>
		[[nodiscard]] bool inherits() const noexcept
		{
			return inherits(get<T>());
		}

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			/* If data is different, types might still be the same, but declared in different binaries. */
			return m_data == other.m_data || name() == other.name();
		}

		constexpr void swap(type_info &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(type_info &a, type_info &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data = nullptr;
	};

	[[nodiscard]] constexpr hash_t hash(const type_info &type) noexcept
	{
		const auto name = type.name();
		return fnv1a(name.data(), name.size());
	}

	template<typename T>
	constexpr type_info type_factory<T>::type() const noexcept
	{
		return type_info{m_data};
	}

	any::any(type_info type, void *ptr) noexcept : m_type(type.m_data), m_storage(ptr, false) {}
	any::any(type_info type, const void *ptr) noexcept : m_type(type.m_data), m_storage(ptr, true) {}
	constexpr type_info any::type() const noexcept { return type_info{m_type}; }

	template<typename T>
	T *any::get() noexcept
	{
		if (type() != type_info::get<T>) [[unlikely]]
			return nullptr;

		if constexpr (std::is_const_v<T>)
			return static_cast<T *>(cdata());
		else
			return static_cast<T *>(data());
	}
	template<typename T>
	std::add_const_t<T> *any::get() const noexcept
	{
		if (type() != type_info::get<T>) [[unlikely]]
			return nullptr;
		return static_cast<T *>(data());
	}

	// clang-format off
	template<typename T>
	std::remove_reference_t<T> &any::as() requires std::is_lvalue_reference_v<T>
	{
		const auto result = as(type_info::get<T>());
		if (result.empty()) [[unlikely]]
		{
			throw type_error(make_error_code(type_errc::INVALID_TYPE),
					 "Cannot convert reference to types that do not "
					 "share a parent-child relationship");
		}
		return *static_cast<T *>(result.data());
	}
	template<typename T>
	std::add_const_t<std::remove_reference_t<T>> &any::as() const requires std::is_lvalue_reference_v<T>
	{
		const auto result = as(type_info::get<T>());
		if (result.empty()) [[unlikely]]
		{
			throw type_error(make_error_code(type_errc::INVALID_TYPE),
					 "Cannot convert reference to types that do not "
					 "share a parent-child relationship");
		}
		return *static_cast<T *>(result.data());
	}
	// clang-format on

	// clang-format off
	template<typename T>
	std::remove_pointer_t<T> *any::as() requires std::is_pointer_v<T>
	{
		return static_cast<T *>(as(type_info::get<T>()).data());
	}
	template<typename T>
	std::add_const_t<std::remove_pointer_t<T>> *any::as() const requires std::is_pointer_v<T>
	{
		return static_cast<const T *>(as(type_info::get<T>()).data());
	}
	// clang-format on

	constexpr type_info any_range::value_type() const noexcept { return type_info{m_data->value_type}; }

	constexpr type_info any_table::value_type() const noexcept { return type_info{m_data->value_type}; }
	constexpr type_info any_table::key_type() const noexcept { return type_info{m_data->key_type}; }
	constexpr type_info any_table::mapped_type() const noexcept { return type_info{m_data->mapped_type}; }

	constexpr type_info any_tuple::element(std::size_t i) const noexcept
	{
		return i < size() ? type_info{m_data->types[i]} : type_info{};
	}

	constexpr type_info any_string::char_type() const noexcept { return type_info{m_data->char_type}; }
	constexpr type_info any_string::value_type() const noexcept { return type_info{m_data->char_type}; }
	constexpr type_info any_string::traits_type() const noexcept { return type_info{m_data->traits_type}; }

	template<typename C>
	C *any_string::chars()
	{
		if (char_type() != type_info::get<C>()) [[unlikely]]
			return nullptr;
		return static_cast<C *>(data());
	}
	template<typename C>
	const C *any_string::chars() const
	{
		if (char_type() != type_info::get<C>()) [[unlikely]]
			return nullptr;
		return static_cast<C *>(data());
	}

	template<typename Sc, typename C, typename T, typename A>
	bool any_string::convert_with(std::basic_string<C, T, A> &dst, const std::locale &l, const A &a) const
	{
		/* Ignore this overload if the type is different. */
		if (char_type() != type_info::get<Sc>()) [[likely]]
			return false;

		if constexpr (std::same_as<Sc, C>)
			dst.assign(static_cast<const Sc *>(data()), size());
		else
		{
			using tmp_alloc_t = typename std::allocator_traits<A>::template rebind_alloc<char>;
			using tmp_traits_t = std::conditional_t<std::same_as<C, char>, T, std::char_traits<char>>;
			using tmp_string_t = std::basic_string<char, tmp_traits_t, tmp_alloc_t>;
			using conv_from_t = std::codecvt<Sc, char, std::mbstate_t>;
			using conv_to_t = std::codecvt<C, char, std::mbstate_t>;

			/* If `Sc` is not `char`, use codecvt to convert to `char`. Otherwise, directly copy the string. */
			auto tmp_buffer = tmp_string_t{tmp_alloc_t{a}};

			if constexpr (std::same_as<Sc, char>)
				tmp_buffer.assign(static_cast<const Sc *>(data()), size());
			else
			{
				auto &conv = std::use_facet<conv_from_t>(l);
				const auto *src_start = static_cast<const Sc *>(data());
				const auto *src_end = src_start + size();
				do_convert(src_start, src_end, tmp_buffer, conv);
			}
			/* If `C` is not `char`, preform a second conversion. Otherwise, use the temporary buffer. */
			if constexpr (std::same_as<C, char>)
				dst = std::move(std::basic_string<C, T, A>{tmp_buffer});
			else
			{
				auto &conv = std::use_facet<conv_to_t>(l);
				const auto *src_end = tmp_buffer.data() + tmp_buffer.size();
				const auto *src_start = tmp_buffer.data();
				do_convert(src_start, src_end, dst, conv);
			}
		}
		return true;
	}
	template<typename C, typename T, typename A>
	expected<std::basic_string<C, T, A>, std::error_code> any_string::as_str(std::nothrow_t, const std::locale &l, const A &a) const
	{
		/* Check if the requested type it is one of the standard character types.
		 * If so, convert using `std::codecvt`. Otherwise, return type error. */
		expected<std::basic_string<C, T, A>, std::error_code> result;
		if (convert_with<char>(*result, l, a) || convert_with<wchar_t>(*result, l, a) || convert_with<char8_t>(*result, l, a) ||
			convert_with<char16_t>(*result, l, a) || convert_with<char32_t>(*result, l, a)) [[likely]]
			return result;

		/* Characters are incompatible and cannot be converted via codecvt. */
		return unexpected{make_error_code(type_errc::INVALID_TYPE)};
	}
	template<typename C, typename T, typename A>
	std::basic_string<C, T, A> any_string::as_str(const std::locale &l, const A &a) const
	{
		/* Check if the requested type it is one of the standard character types.
		 * If so, convert using `std::codecvt`. Otherwise throw. */
		std::basic_string<C, T, A> result;
		if (convert_with<char>(result, l, a) || convert_with<wchar_t>(result, l, a) || convert_with<char8_t>(result, l, a) ||
			convert_with<char16_t>(result, l, a) || convert_with<char32_t>(result, l, a)) [[likely]]
			return result;

		/* Characters are incompatible and cannot be converted via codecvt. */
		throw type_error(make_error_code(type_errc::INVALID_TYPE), "Cannot convert to requested string type");
	}

	// clang-format off
	extern template SEK_API_IMPORT expected<std::basic_string<char>, std::error_code> any_string::as_str(
		std::nothrow_t, const std::locale &, const typename std::basic_string<char>::allocator_type &) const;
	extern template SEK_API_IMPORT expected<std::basic_string<wchar_t>, std::error_code> any_string::as_str(
		std::nothrow_t, const std::locale &, const typename std::basic_string<wchar_t>::allocator_type &) const;

	extern template SEK_API_IMPORT std::basic_string<char> any_string::as_str(
		const std::locale &, const typename std::basic_string<char>::allocator_type &) const;
	extern template SEK_API_IMPORT std::basic_string<wchar_t> any_string::as_str(
		const std::locale &, const typename std::basic_string<wchar_t>::allocator_type &) const;
	// clang-format on

	/* Type names for reflection types. */
	template<>
	struct type_name<any>
	{
		constexpr static std::string_view value = "sek::any";
	};
	template<>
	struct type_name<type_info>
	{
		constexpr static std::string_view value = "sek::type_info";
	};

	/** Returns the type info of an object's type. Equivalent to `type_info::get<T>()`. */
	template<typename T>
	[[nodiscard]] constexpr type_info type_of(T &&) noexcept
	{
		return type_info::get<T>();
	}

	namespace literals
	{
		/** Retrieves a reflected type from the runtime database. */
		[[nodiscard]] inline type_info operator""_type(const char *str, std::size_t n)
		{
			return type_info::get({str, n});
		}
	}	 // namespace literals

	/** @brief Helper type used to check if a type has been exported via `SEK_EXTERN_TYPE_INFO`. */
	template<typename T>
	struct is_exported_type : std::false_type
	{
	};
	/** @brief Alias for `is_exported_type<T>::value`. */
	template<typename T>
	constexpr static auto is_exported_type_v = is_exported_type<T>::value;
}	 // namespace sek

template<>
struct std::hash<sek::type_info>
{
	[[nodiscard]] constexpr std::size_t operator()(const sek::type_info &info) const noexcept
	{
		return sek::hash(info);
	}
};

/** Macro used to declare an instance of type info for type `T` as extern.
 * @note Type must be exported via `SEK_EXPORT_TYPE_INFO`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE_INFO(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE_INFO(my_type)
 * @endcode*/
#define SEK_EXTERN_TYPE_INFO(T)                                                                                        \
	template<>                                                                                                         \
	struct sek::is_exported_type<T> : std::true_type                                                                   \
	{                                                                                                                  \
	};                                                                                                                 \
	extern template SEK_API_IMPORT sek::detail::type_data *sek::detail::type_data::instance<T>() noexcept;

/** Macro used to export instance of type info for type `T`.
 * @note Type must be declared as `extern` via `SEK_EXTERN_TYPE_INFO`.
 *
 * @example
 * @code{.cpp}
 * // my_type.hpp
 * struct my_type {};
 * SEK_EXTERN_TYPE_INFO(my_type)
 *
 * // my_type.cpp
 * SEK_EXPORT_TYPE_INFO(my_type)
 * @endcode */
#define SEK_EXPORT_TYPE_INFO(T)                                                                                        \
	template SEK_API_EXPORT sek::detail::type_data *sek::detail::type_data::instance<T>() noexcept;

/* Type exports for reflection types */
SEK_EXTERN_TYPE_INFO(sek::any);
SEK_EXTERN_TYPE_INFO(sek::type_info);
