/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include <functional>
#include <memory>

#include "../../dense_map.hpp"
#include "../../property.hpp"
#include "../../type_name.hpp"
#include "fwd.hpp"
#include "traits.hpp"

namespace sek::detail
{
	template<typename From, typename To>
	struct default_conv
	{
		[[nodiscard]] constexpr To operator()(From &value) const { return static_cast<To>(value); }
		[[nodiscard]] constexpr To operator()(const From &value) const { return static_cast<To>(value); }
	};

	/* Data references are lazy-evaluated to enable dependency cycles. */
	struct type_handle_t
	{
		constexpr type_handle_t() noexcept = default;
		template<typename T>
		constexpr explicit type_handle_t(type_selector_t<T>) noexcept;

		[[nodiscard]] constexpr type_info operator()() const noexcept;

		type_info (*get)() noexcept = nullptr;
	};

	template<typename T>
	constexpr static auto type_handle = type_handle_t{type_selector<T>};

	/* Simple type-erased object wrapper. */
	struct generic_type_data
	{
		generic_type_data(const generic_type_data &) = delete;
		generic_type_data &operator=(const generic_type_data &) = delete;

		constexpr generic_type_data() noexcept = default;
		constexpr generic_type_data(generic_type_data &&other) noexcept { swap(other); }
		constexpr generic_type_data &operator=(generic_type_data &&other) noexcept
		{
			swap(other);
			return *this;
		}
		constexpr ~generic_type_data()
		{
			if (destroy_func != nullptr) [[likely]]
				destroy_func(data);
		}

		template<typename T, typename... Args>
		constexpr void init(Args &&...args)
		{
			if constexpr (!std::is_empty_v<T>)
			{
				if constexpr (std::is_aggregate_v<T>)
					data = new T{std::forward<Args>(args)...};
				else
					data = new T(std::forward<Args>(args)...);
				destroy_func = +[](void *data) { delete static_cast<T *>(data); };
			}
		}

		constexpr void swap(generic_type_data &other) noexcept
		{
			std::swap(data, other.data);
			std::swap(destroy_func, other.destroy_func);
		}

		void (*destroy_func)(void *) = nullptr;
		void *data = nullptr;
	};

	struct attr_data : generic_type_data
	{
		template<typename T, typename... Args>
		[[nodiscard]] static attr_data make_instance(Args &&...);

		constexpr attr_data() noexcept = default;
		constexpr attr_data(attr_data &&other) noexcept { swap(other); }
		constexpr attr_data &operator=(attr_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(attr_data &other) noexcept
		{
			generic_type_data::swap(other);
			std::swap(get_func, other.get_func);
			std::swap(type, other.type);
		}

		[[nodiscard]] inline any get() const;

		any (*get_func)(const void *) = nullptr;
		type_handle_t type;
	};
	struct base_data
	{
		template<typename T, typename P>
		[[nodiscard]] constexpr static base_data make_instance() noexcept;

		constexpr base_data() noexcept = default;
		constexpr base_data(base_data &&other) noexcept { swap(other); }
		constexpr base_data &operator=(base_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(base_data &other) noexcept
		{
			std::swap(cast, other.cast);
			std::swap(type, other.type);
		}

		const void *(*cast)(const void *) = nullptr;
		type_handle_t type;
	};
	struct conv_data
	{
		template<typename From, typename To, typename Conv = default_conv<From, To>>
		[[nodiscard]] constexpr static conv_data make_instance() noexcept;

		constexpr conv_data() noexcept = default;
		constexpr conv_data(conv_data &&other) noexcept { swap(other); }
		constexpr conv_data &operator=(conv_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(conv_data &other) noexcept
		{
			std::swap(convert, other.convert);
			std::swap(type, other.type);
		}

		any (*convert)(const any &) = nullptr;
		type_handle_t type;
	};

	using attr_table = dense_map<std::string_view, attr_data>;
	using base_table = dense_map<std::string_view, base_data>;
	using conv_table = dense_map<std::string_view, conv_data>;

	struct func_arg
	{
		template<typename T>
		constexpr explicit func_arg(type_selector_t<T>) noexcept
		{
			type = type_handle<std::remove_cv_t<T>>;
			is_const = std::is_const_v<T>;
		}

		[[nodiscard]] constexpr bool operator==(const func_arg &other) const noexcept;

		type_handle_t type; /* Actual type of the argument. */
		bool is_const;		/* Is the argument type const-qualified. */
	};

	template<typename... Args>
	[[nodiscard]] constexpr auto make_func_args(type_seq_t<Args...>) noexcept
	{
		using array_t = std::array<func_arg, sizeof...(Args)>;
		return func_arg{func_arg(type_selector<std::remove_reference_t<Args>>)...};
	}
	template<template_instance<type_seq_t> Args>
	[[nodiscard]] constexpr auto make_func_args() noexcept
	{
		return make_func_args(Args{});
	}
	template<template_instance<type_seq_t> Args>
	constexpr static auto func_args = make_func_args<Args>();

	struct dtor_data : generic_type_data
	{
		template<typename T>
		[[nodiscard]] constexpr static dtor_data make_instance() noexcept;
		template<typename T, auto F>
		[[nodiscard]] constexpr static dtor_data make_instance() noexcept;
		template<typename T, typename F, typename... Args>
		[[nodiscard]] static dtor_data make_instance(Args &&...);

		constexpr dtor_data() noexcept = default;
		constexpr dtor_data(dtor_data &&other) noexcept { swap(other); }
		constexpr dtor_data &operator=(dtor_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(dtor_data &other) noexcept
		{
			generic_type_data::swap(other);
			std::swap(invoke_func, other.invoke_func);
		}

		inline void invoke(void *) const;

		void (*invoke_func)(const void *, void *) = nullptr;
	};
	struct ctor_data : generic_type_data
	{
		template<typename T, typename... Args>
		[[nodiscard]] constexpr static ctor_data make_instance(type_seq_t<Args...>) noexcept;
		template<typename T, auto F, typename... Args>
		[[nodiscard]] constexpr static ctor_data make_instance(type_seq_t<Args...>) noexcept;
		template<typename T, typename F, typename... Args, typename... FArgs>
		[[nodiscard]] static ctor_data make_instance(type_seq_t<Args...>, FArgs &&...);

		template<typename... Args, typename F, std::size_t... Is>
		[[nodiscard]] static any invoke_impl(std::index_sequence<Is...>, F &&, std::span<any>);
		template<typename... Args, typename F>
		[[nodiscard]] static any invoke_impl(type_seq_t<Args...>, F &&, std::span<any>);

		constexpr ctor_data() noexcept = default;
		constexpr ctor_data(ctor_data &&other) noexcept { swap(other); }
		constexpr ctor_data &operator=(ctor_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(ctor_data &other) noexcept
		{
			generic_type_data::swap(other);
			std::swap(invoke_func, other.invoke_func);
			std::swap(args, other.args);
		}

		[[nodiscard]] inline any invoke(std::span<any>) const;

		any (*invoke_func)(const void *, std::span<any>) = nullptr;
		std::span<const func_arg> args;
	};

	struct const_data
	{
		template<auto V>
		[[nodiscard]] constexpr static const_data make_instance(std::string_view name) noexcept;
		template<typename F>
		[[nodiscard]] constexpr static const_data make_instance(std::string_view name) noexcept;

		constexpr const_data() noexcept = default;
		constexpr const_data(std::string_view name) noexcept : name(name) {}

		constexpr const_data(const_data &&other) noexcept { swap(other); }
		constexpr const_data &operator=(const_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(const_data &other) noexcept
		{
			using std::swap;
			swap(get, other.get);
			swap(type, other.type);
			swap(name, other.name);
			swap(attributes, other.attributes);
		}

		any (*get)() = nullptr;
		type_handle_t type;
		std::string_view name;
		attr_table attributes;
	};
	struct func_overload : generic_type_data
	{
		template<auto F>
		[[nodiscard]] constexpr static func_overload make_instance() noexcept;
		template<typename F, typename... FArgs>
		[[nodiscard]] static func_overload make_instance(FArgs &&...);

		template<typename I, typename... Args, typename F, std::size_t... Is>
		constexpr static decltype(auto) invoke_impl(std::index_sequence<Is...>, F &&, I *, std::span<any>);
		template<typename I, typename... Args, typename F>
		constexpr static decltype(auto) invoke_impl(type_seq_t<Args...>, F &&, I *, std::span<any>);
		template<typename... Args, typename F, std::size_t... Is>
		constexpr static decltype(auto) invoke_impl(std::index_sequence<Is...>, F &&, std::span<any>);
		template<typename... Args, typename F>
		constexpr static decltype(auto) invoke_impl(type_seq_t<Args...>, F &&, std::span<any>);

		constexpr func_overload() noexcept = default;
		constexpr func_overload(func_overload &&other) noexcept { swap(other); }
		constexpr func_overload &operator=(func_overload &&other) noexcept
		{
			swap(other);
			return *this;
		}

		[[nodiscard]] inline any invoke(const void *, std::span<any>) const;

		[[nodiscard]] constexpr bool operator==(const func_overload &other) const noexcept;

		constexpr void swap(func_overload &other) noexcept
		{
			generic_type_data::swap(other);

			using std::swap;
			swap(attributes, other.attributes);
			swap(invoke_func, other.invoke_func);
			swap(is_const, other.is_const);
			swap(args, other.args);
			swap(inst, other.inst);
			swap(ret, other.ret);
		}

		any (*invoke_func)(const void *, const void *, std::span<any>) = nullptr;

		bool is_const = false;
		std::span<const func_arg> args;
		type_handle_t inst;
		type_handle_t ret;

		attr_table attributes;
	};
	struct func_data
	{
		using overloads_t = std::vector<func_overload>;

		constexpr func_data() noexcept = default;
		constexpr func_data(std::string_view name) noexcept : name(name) {}

		constexpr func_data(func_data &&other) noexcept { swap(other); }
		constexpr func_data &operator=(func_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(func_data &other) noexcept
		{
			std::swap(name, other.name);
			std::swap(overloads, other.overloads);
		}

		template<typename P>
		[[nodiscard]] typename overloads_t::iterator overload(P &&pred)
		{
			return std::ranges::find_if(overloads, std::forward<P>(pred));
		}
		template<typename P>
		[[nodiscard]] typename overloads_t::const_iterator overload(P &&pred) const
		{
			return std::ranges::find_if(overloads, std::forward<P>(pred));
		}

		std::string_view name;
		overloads_t overloads;
	};
	struct prop_data : generic_type_data
	{
		template<typename T, typename G, typename S, typename... GArgs, typename... SArgs>
		[[nodiscard]] static func_data make_instance(std::tuple<GArgs &&...> g_args, std::tuple<SArgs &&...> s_args);
		template<typename T, auto G, auto S>
		[[nodiscard]] constexpr static func_data make_instance() noexcept;

		constexpr prop_data() noexcept = default;
		constexpr prop_data(prop_data &&other) noexcept { swap(other); }
		constexpr prop_data &operator=(prop_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(prop_data &other) noexcept
		{
			generic_type_data::swap(other);

			using std::swap;
			swap(get_func, other.get_func);
			swap(set_func, other.set_func);
			swap(name, other.name);
			swap(attributes, other.attributes);
		}

		[[nodiscard]] inline any get() const;
		inline void set(const any &) const;

		any (*get_func)(const void *) = nullptr;
		void (*set_func)(void *, const any &) = nullptr;

		std::string_view name;
		attr_table attributes;
	};

	using const_table = dense_map<std::string_view, const_data>;
	using func_table = dense_map<std::string_view, func_data>;
	using prop_table = dense_map<std::string_view, prop_data>;

	struct any_vtable
	{
		template<typename T>
		constexpr static any_vtable make_instance() noexcept;

		void (*copy_init)(const any &, any &) = nullptr;
		void (*copy_assign)(const any &, any &) = nullptr;

		bool (*cmp_eq)(const void *, const void *) = nullptr;
		bool (*cmp_lt)(const void *, const void *) = nullptr;
		bool (*cmp_le)(const void *, const void *) = nullptr;
		bool (*cmp_gt)(const void *, const void *) = nullptr;
		bool (*cmp_ge)(const void *, const void *) = nullptr;
	};

	/* TODO: Implement generic range, table, tuple & string type proxies. */

	struct type_data
	{
		template<typename T>
		[[nodiscard]] static type_data make_instance() noexcept;

		void reset() noexcept { reset_func(this); }

		void (*reset_func)(type_data *) noexcept;
		std::string_view name;

		bool is_void = false;
		bool is_empty = false;
		bool is_signed = false;
		bool is_unsigned = false;
		bool is_floating = false;

		attr_table attributes;
		const_table constants;
		base_table parents;

		dtor_data destructor;
		std::vector<ctor_data> constructors;

		func_table functions;
		prop_table properties;

		type_handle_t enum_type;
		conv_table conversions;

		any_vtable any_funcs;
	};
}	 // namespace sek::detail