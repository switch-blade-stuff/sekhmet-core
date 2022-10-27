/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include <functional>
#include <memory>

#include "../../dense_map.hpp"
#include "../../dense_set.hpp"
#include "../../meta.hpp"
#include "../../type_name.hpp"
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

		[[nodiscard]] constexpr hash_t operator()(std::string_view type) const noexcept;
		[[nodiscard]] constexpr hash_t operator()(const type_data *type) const noexcept;
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
				destroy_func = +[](void *ptr) { delete static_cast<T *>(ptr); };
			}
		}

		constexpr void swap(generic_type_data &other) noexcept
		{
			std::swap(data, other.data);
			std::swap(destroy_func, other.destroy_func);
		}

		void *data = nullptr;
		void (*destroy_func)(void *) = nullptr;
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

		[[nodiscard]] constexpr hash_t operator()(const attr_data &data) const noexcept
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

		[[nodiscard]] constexpr hash_t operator()(const base_data &data) const noexcept
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
			std::swap(type, other.type);
		}

		any (*convert)(any) = nullptr;
		type_handle type;
	};
	struct conv_hash : type_data_hash
	{
		using type_data_hash::operator();

		[[nodiscard]] constexpr hash_t operator()(const conv_data &data) const noexcept
		{
			return operator()(data.type.get());
		}
	};
	struct conv_cmp : type_data_cmp
	{
		using type_data_cmp::operator();

		[[nodiscard]] constexpr bool operator()(const conv_data &a, const conv_data &b) const noexcept
		{
			return &a == &b || operator()(a.type.get(), b.type.get());
		}
		[[nodiscard]] constexpr bool operator()(const type_data *a, const conv_data &b) const noexcept
		{
			return operator()(a, b.type.get());
		}
		[[nodiscard]] constexpr bool operator()(const conv_data &a, const type_data *b) const noexcept
		{
			return operator()(a.type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(const conv_data &a, std::string_view b) const noexcept
		{
			return operator()(a.type.get(), b);
		}
		[[nodiscard]] constexpr bool operator()(std::string_view a, const conv_data &b) const noexcept
		{
			return operator()(a, b.type.get());
		}
	};
	using conv_table = dense_set<conv_data, conv_hash, conv_cmp>;

	struct const_data
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
			using std::swap;
			swap(get, other.get);
			swap(attributes, other.attributes);
			swap(type, other.type);
		}

		any (*get)() = nullptr;
		attr_table attributes;
		std::string_view name;
		type_handle type;
	};
	struct const_hash
	{
		typedef std::true_type is_transparent;

		[[nodiscard]] constexpr hash_t operator()(std::string_view name) const noexcept
		{
			return fnv1a(name.data(), name.size());
		}
		[[nodiscard]] constexpr hash_t operator()(const const_data &data) const noexcept
		{
			return operator()(data.name);
		}
	};
	struct const_cmp
	{
		typedef std::true_type is_transparent;

		[[nodiscard]] constexpr bool operator()(const const_data &a, const const_data &b) const noexcept
		{
			return &a == &b || a.name == b.name;
		}
		[[nodiscard]] constexpr bool operator()(const const_data &a, std::string_view b) const noexcept
		{
			return a.name == b;
		}
		[[nodiscard]] constexpr bool operator()(std::string_view a, const const_data &b) const noexcept
		{
			return a == b.name;
		}
	};
	using const_table = dense_set<const_data, const_hash, const_cmp>;

	struct func_arg_data
	{
		template<typename T>
		constexpr explicit func_arg_data(type_selector_t<T>) noexcept
		{
			type = type_handle{type_selector<std::remove_cv_t<T>>};
			is_const = std::is_const_v<T>;
		}

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

		template<typename T, typename... Args, typename F, std::size_t... Is>
		static any invoke_impl(std::index_sequence<Is...>, F &&, std::span<any>);
		template<typename T, typename... Args, typename F>
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
		std::span<func_arg_data> args;
	};

	struct func_data : generic_type_data
	{
		template<typename T, auto F>
		[[nodiscard]] constexpr static func_data make_instance() noexcept;
		template<typename T, typename F, typename... FArgs>
		[[nodiscard]] static func_data make_instance(FArgs &&...);

		template<typename I, typename... Args, typename F, std::size_t... Is>
		constexpr static decltype(auto) invoke_impl(std::index_sequence<Is...>, F &&, I *, std::span<any>);
		template<typename I, typename... Args, typename F>
		constexpr static decltype(auto) invoke_impl(type_seq_t<Args...>, F &&, I *, std::span<any>);
		template<typename... Args, typename F, std::size_t... Is>
		constexpr static decltype(auto) invoke_impl(std::index_sequence<Is...>, F &&, std::span<any>);
		template<typename... Args, typename F>
		constexpr static decltype(auto) invoke_impl(type_seq_t<Args...>, F &&, std::span<any>);

		constexpr func_data() noexcept = default;
		constexpr func_data(func_data &&other) noexcept { swap(other); }
		constexpr func_data &operator=(func_data &&other) noexcept
		{
			swap(other);
			return *this;
		}

		constexpr void swap(func_data &other) noexcept
		{
			generic_type_data::swap(other);

			using std::swap;
			swap(invoke_func, other.invoke_func);
			swap(args, other.args);
			swap(ret, other.ret);
			swap(attributes, other.attributes);
			swap(is_const, other.is_const);
			swap(is_static, other.is_static);
		}

		inline any invoke(const void *, std::span<any>) const;

		any (*invoke_func)(const void *, const void *, std::span<any>) = nullptr;

		std::span<func_arg_data> args;
		type_handle ret;

		attr_table attributes;

		bool is_const = false;
		bool is_static = false;
	};
	struct prop_data : generic_type_data
	{
		/* TODO: Implement property initialization. */

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
			swap(attributes, other.attributes);
		}

		[[nodiscard]] inline any get() const;
		inline void set(const any &);

		any (*get_func)(const void *) = nullptr;
		void (*set_func)(void *, const any &) = nullptr;

		attr_table attributes;
	};

	class range_type_iterator;
	class table_type_iterator;

	/* TODO: Implement range, table & string type data. */
	struct range_type_data
	{
		template<typename T>
		constexpr static range_type_data make_instance() noexcept;

		type_handle value_type;

		bool (*empty)(const void *) = nullptr;
		std::size_t (*size)(const void *) = nullptr;

		range_type_iterator (*begin)(const any &) = nullptr;
		range_type_iterator (*end)(const any &) = nullptr;
		range_type_iterator (*rbegin)(const any &) = nullptr;
		range_type_iterator (*rend)(const any &) = nullptr;

		any (*front)(const any &) = nullptr;
		any (*back)(const any &) = nullptr;
		any (*at)(const any &, std::size_t) = nullptr;
	};
	struct table_type_data
	{
		template<typename T>
		constexpr static table_type_data make_instance() noexcept;

		type_handle value_type;
		type_handle key_type;
		type_handle mapped_type;

		bool (*empty)(const void *) = nullptr;
		std::size_t (*size)(const void *) = nullptr;
		bool (*contains)(const void *, const any &) = nullptr;

		table_type_iterator (*begin)(const any &) = nullptr;
		table_type_iterator (*end)(const any &) = nullptr;
		table_type_iterator (*rbegin)(const any &) = nullptr;
		table_type_iterator (*rend)(const any &) = nullptr;
		table_type_iterator (*find)(const any &, const any &) = nullptr;

		any (*at)(const any &, const any &) = nullptr;
	};
	struct tuple_type_data
	{
		template<typename T>
		struct getter_t
		{
			constexpr static auto size = std::tuple_size_v<T>;

			constexpr getter_t() noexcept { init(); }

			[[nodiscard]] any operator()(T &t, std::size_t i) const;

			template<std::size_t I = 0>
			constexpr void init() noexcept;

			any (*table[size])(T &);
		};

		template<typename T>
		constexpr static tuple_type_data make_instance() noexcept;
		template<typename T>
		constexpr static auto make_type_array() noexcept;

		std::span<type_handle> types;
		any (*get)(const any &, std::size_t) = nullptr;
	};
	struct string_type_data
	{
		template<typename T>
		constexpr static string_type_data make_instance() noexcept;

		type_handle char_type;
		type_handle traits_type;

		bool (*empty)(const void *) = nullptr;
		std::size_t (*size)(const void *) = nullptr;
		const void *(*data)(const any &) = nullptr;
	};

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

		dtor_data dtor;
		std::vector<ctor_data> constructors;

		dense_map<std::string_view, prop_data> properties;
		dense_map<std::string_view, std::vector<func_data>> functions;

		type_handle enum_type;
		conv_table conversions;

		const range_type_data *range_data = nullptr;
		const table_type_data *table_data = nullptr;
		const tuple_type_data *tuple_data = nullptr;
		const string_type_data *string_data = nullptr;
		any_vtable any_funcs = {};
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

	constexpr hash_t type_data_hash::operator()(std::string_view type) const noexcept
	{
		return fnv1a(type.data(), type.size());
	}
	constexpr hash_t type_data_hash::operator()(const type_data *type) const noexcept { return operator()(type->name); }

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
}	 // namespace sek::detail