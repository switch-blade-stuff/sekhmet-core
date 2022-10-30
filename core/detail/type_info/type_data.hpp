/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include <functional>
#include <memory>

#include "../../dense_set.hpp"
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

	/* Data references are lazy-evaluated to allow dependency loops between types. */
	struct type_handle
	{
		constexpr type_handle() noexcept = default;
		template<typename T>
		constexpr explicit type_handle(type_selector_t<T>) noexcept;

		[[nodiscard]] constexpr type_data *operator->() const noexcept { return get(); }
		[[nodiscard]] constexpr type_data &operator*() const noexcept { return *get(); }

		type_data *(*get)() noexcept = nullptr;
	};

	struct type_data_hash
	{
		typedef std::true_type is_transparent;

		[[nodiscard]] constexpr std::size_t operator()(std::string_view type) const noexcept;
		[[nodiscard]] constexpr std::size_t operator()(const type_data *type) const noexcept;
	};
	struct type_data_cmp
	{
		typedef std::true_type is_transparent;

		[[nodiscard]] constexpr bool operator()(const type_data *a, const type_data *b) const noexcept;
		[[nodiscard]] constexpr bool operator()(const type_data *a, std::string_view b) const noexcept;
		[[nodiscard]] constexpr bool operator()(std::string_view a, const type_data *b) const noexcept;
	};

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
		type_handle type;
	};
	struct attr_hash : type_data_hash
	{
		using type_data_hash::operator();

		[[nodiscard]] constexpr std::size_t operator()(const attr_data &data) const noexcept
		{
			return operator()(data.type.get());
		}
	};
	struct attr_cmp : type_data_cmp
	{
		using type_data_cmp::operator();

		[[nodiscard]] constexpr bool operator()(const attr_data &a, const attr_data &b) const noexcept
		{
			return &a == &b || operator()(a.type.get(), b.type.get());
		}
		[[nodiscard]] constexpr bool operator()(const type_data *a, const attr_data &b) const noexcept
		{
			return operator()(a, b.type.get());
		}
		[[nodiscard]] constexpr bool operator()(const attr_data &a, const type_data *b) const noexcept
		{
			return operator()(a.type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(const attr_data &a, std::string_view b) const noexcept
		{
			return operator()(a.type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(std::string_view a, const attr_data &b) const noexcept
		{
			return operator()(a, b.type.get());
		}
	};
	using attr_table = dense_set<attr_data, attr_hash, attr_cmp>;

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
		type_handle type;
	};
	struct base_hash : type_data_hash
	{
		using type_data_hash::operator();

		[[nodiscard]] constexpr std::size_t operator()(const base_data &data) const noexcept
		{
			return operator()(data.type.get());
		}
	};
	struct base_cmp : type_data_cmp
	{
		using type_data_cmp::operator();

		[[nodiscard]] constexpr bool operator()(const base_data &a, const base_data &b) const noexcept
		{
			return &a == &b || operator()(a.type.get(), b.type.get());
		}
		[[nodiscard]] constexpr bool operator()(const type_data *a, const base_data &b) const noexcept
		{
			return operator()(a, b.type.get());
		}
		[[nodiscard]] constexpr bool operator()(const base_data &a, const type_data *b) const noexcept
		{
			return operator()(a.type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(const base_data &a, std::string_view b) const noexcept
		{
			return operator()(a.type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(std::string_view a, const base_data &b) const noexcept
		{
			return operator()(a, b.type.get());
		}
	};
	using base_table = dense_set<base_data, base_hash, base_cmp>;

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
			std::swap(from_type, other.from_type);
			std::swap(to_type, other.to_type);
		}

		any (*convert)(const any &) = nullptr;
		type_handle from_type;
		type_handle to_type;
	};
	struct conv_hash : type_data_hash
	{
		using type_data_hash::operator();

		[[nodiscard]] constexpr std::size_t operator()(const conv_data &data) const noexcept
		{
			return operator()(data.to_type.get());
		}
	};
	struct conv_cmp : type_data_cmp
	{
		using type_data_cmp::operator();

		[[nodiscard]] constexpr bool operator()(const conv_data &a, const conv_data &b) const noexcept
		{
			return &a == &b || operator()(a.to_type.get(), b.to_type.get());
		}
		[[nodiscard]] constexpr bool operator()(const type_data *a, const conv_data &b) const noexcept
		{
			return operator()(a, b.to_type.get());
		}
		[[nodiscard]] constexpr bool operator()(const conv_data &a, const type_data *b) const noexcept
		{
			return operator()(a.to_type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(const conv_data &a, std::string_view b) const noexcept
		{
			return operator()(a.to_type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(std::string_view a, const conv_data &b) const noexcept
		{
			return operator()(a, b.to_type.get());
		}
	};
	using conv_table = dense_set<conv_data, conv_hash, conv_cmp>;

	struct func_arg_data
	{
		template<typename T>
		constexpr explicit func_arg_data(type_selector_t<T>) noexcept
		{
			type = type_handle{type_selector<std::remove_cv_t<T>>};
			is_const = std::is_const_v<T>;
		}

		[[nodiscard]] constexpr bool operator==(const func_arg_data &other) const noexcept;

		type_handle type; /* Actual type of the argument. */
		bool is_const;	  /* Is the argument type const-qualified. */
	};

	template<typename... Args>
	[[nodiscard]] constexpr auto make_args_array(type_seq_t<Args...>) noexcept
	{
		using array_t = std::array<func_arg_data, sizeof...(Args)>;
		return array_t{func_arg_data(type_selector<std::remove_reference_t<Args>>)...};
	}
	template<template_instance<type_seq_t> Args>
	[[nodiscard]] constexpr auto make_args_array() noexcept
	{
		return make_args_array(Args{});
	}
	template<template_instance<type_seq_t> Args>
	constexpr static auto arg_types_array = make_args_array<Args>();

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
		static any invoke_impl(std::index_sequence<Is...>, F &&, std::span<any>);
		template<typename... Args, typename F>
		static any invoke_impl(type_seq_t<Args...>, F &&, std::span<any>);

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
		std::span<const func_arg_data> args;
	};

	struct type_member_data
	{
		constexpr type_member_data() noexcept = default;
		constexpr type_member_data(std::string_view name) noexcept : name(name) {}

		constexpr void swap(type_member_data &other) noexcept { std::swap(name, other.name); }

		std::string_view name;
	};
	struct member_hash
	{
		typedef std::true_type is_transparent;

		[[nodiscard]] constexpr std::size_t operator()(std::string_view name) const noexcept
		{
			return fnv1a(name.data(), name.size());
		}
		[[nodiscard]] constexpr std::size_t operator()(const type_member_data &data) const noexcept
		{
			return operator()(data.name);
		}
	};
	struct member_cmp
	{
		typedef std::true_type is_transparent;

		[[nodiscard]] constexpr bool operator()(const type_member_data &a, const type_member_data &b) const noexcept
		{
			return &a == &b || a.name == b.name;
		}
		[[nodiscard]] constexpr bool operator()(const type_member_data &a, std::string_view b) const noexcept
		{
			return a.name == b;
		}
		[[nodiscard]] constexpr bool operator()(std::string_view a, const type_member_data &b) const noexcept
		{
			return a == b.name;
		}
	};

	struct const_data : type_member_data
	{
		template<auto V>
		[[nodiscard]] constexpr static const_data make_instance(std::string_view name) noexcept;
		template<typename F>
		[[nodiscard]] constexpr static const_data make_instance(std::string_view name) noexcept;

		constexpr const_data() noexcept = default;
		constexpr const_data(const_data &&other) noexcept { swap(other); }
		constexpr const_data &operator=(const_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(const_data &other) noexcept
		{
			type_member_data::swap(other);

			using std::swap;
			swap(attributes, other.attributes);
			swap(get, other.get);
			swap(type, other.type);
		}

		mutable attr_table attributes;
		any (*get)() = nullptr;
		type_handle type;
	};
	using const_table = dense_set<const_data, member_hash, member_cmp>;

	struct signature_data
	{
		constexpr void swap(signature_data &other) noexcept
		{
			using std::swap;
			swap(args, other.args);
			swap(instance_type, other.instance_type);
			swap(is_const, other.is_const);
		}

		[[nodiscard]] constexpr bool operator==(const signature_data &other) const noexcept;

		std::span<const func_arg_data> args;

		type_handle instance_type;
		bool is_const = false;
	};
	struct func_overload : generic_type_data, signature_data
	{
		template<typename T, auto F>
		[[nodiscard]] constexpr static func_overload make_instance() noexcept;
		template<typename T, typename F, typename... FArgs>
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

		inline any invoke(const void *, std::span<any>) const;

		constexpr void swap(func_overload &other) noexcept
		{
			generic_type_data::swap(other);
			signature_data::swap(other);

			using std::swap;
			swap(invoke_func, other.invoke_func);
			swap(ret, other.ret);
			swap(attributes, other.attributes);
		}

		any (*invoke_func)(const void *, const void *, std::span<any>) = nullptr;
		type_handle ret;

		mutable attr_table attributes;
	};
	struct func_data : type_member_data
	{
		using overloads_t = std::vector<func_overload>;

		constexpr func_data() noexcept = default;
		constexpr func_data(std::string_view name) noexcept : type_member_data(name) {}

		constexpr func_data(func_data &&other) noexcept { swap(other); }
		constexpr func_data &operator=(func_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(func_data &other) noexcept
		{
			type_member_data::swap(other);
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

		mutable overloads_t overloads;
	};
	using func_table = dense_set<func_data, member_hash, member_cmp>;

	struct prop_data : generic_type_data, type_member_data
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
			type_member_data::swap(other);

			using std::swap;
			swap(attributes, other.attributes);
			swap(get_func, other.get_func);
			swap(set_func, other.set_func);
		}

		[[nodiscard]] inline any get() const;
		inline void set(const any &) const;

		mutable attr_table attributes;
		any (*get_func)(const void *) = nullptr;
		void (*set_func)(void *, const any &) = nullptr;
	};
	using prop_table = dense_set<prop_data, member_hash, member_cmp>;

	/* TODO: Implement generic range, table, tuple & string type proxies. */

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

	struct type_data
	{
		template<typename T>
		[[nodiscard]] static type_data make_instance() noexcept;
		template<typename T>
		static type_data *instance() noexcept;

		void reset() noexcept { reset_func(this); }

		void (*reset_func)(type_data *) noexcept;

		std::string_view name;

		bool is_void = false;
		bool is_empty = false;
		bool is_nullptr = false;

		bool is_int = false;
		bool is_uint = false;
		bool is_float = false;

		attr_table attributes;
		const_table constants;
		base_table parents;

		dtor_data destructor;
		std::vector<ctor_data> constructors;

		func_table functions;
		prop_table properties;

		type_handle enum_type;
		conv_table conversions;

		any_vtable any_funcs;
	};

	template<typename T>
	constexpr type_handle::type_handle(type_selector_t<T>) noexcept : get(type_data::instance<T>)
	{
	}
	template<typename T>
	type_data *type_data::instance() noexcept
	{
		static type_data value = make_instance<T>();
		return &value;
	}

	constexpr std::size_t type_data_hash::operator()(std::string_view type) const noexcept
	{
		return fnv1a(type.data(), type.size());
	}
	constexpr std::size_t type_data_hash::operator()(const type_data *type) const noexcept
	{
		return operator()(type->name);
	}

	constexpr bool type_data_cmp::operator()(const type_data *a, const type_data *b) const noexcept
	{
		return a == b || a->name == b->name;
	}
	constexpr bool type_data_cmp::operator()(const type_data *a, std::string_view b) const noexcept
	{
		return a->name == b;
	}
	constexpr bool type_data_cmp::operator()(std::string_view a, const type_data *b) const noexcept
	{
		return a == b->name;
	}

	constexpr bool func_arg_data::operator==(const func_arg_data &other) const noexcept
	{
		return is_const == other.is_const && (type.get == other.type.get || type->name == other.type->name);
	}
	constexpr bool signature_data::operator==(const signature_data &other) const noexcept
	{
		return (instance_type.get == other.instance_type.get || instance_type->name == other.instance_type->name) &&
			   is_const == other.is_const && std::ranges::equal(args, other.args);
	}
}	 // namespace sek::detail