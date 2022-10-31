/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "type_data.hpp"
#include "type_error.hpp"

namespace sek
{
	namespace detail
	{
		template<typename Value, typename Container>
		class type_info_view;
		template<typename Value, typename Iter>
		class type_info_iterator;

		template<typename Value>
		class type_info_ptr
		{
			template<typename, typename>
			friend class type_info_iterator;

		public:
			typedef Value element_type;
			typedef const Value *pointer;

		private:
			template<typename... Args>
			constexpr type_info_ptr(Args &&...args) : m_value(std::forward<Args>(args)...)
			{
			}

		public:
			type_info_ptr() = delete;

			constexpr type_info_ptr(const type_info_ptr &) noexcept = default;
			constexpr type_info_ptr &operator=(const type_info_ptr &) noexcept = default;
			constexpr type_info_ptr(type_info_ptr &&) noexcept = default;
			constexpr type_info_ptr &operator=(type_info_ptr &&) noexcept = default;

			[[nodiscard]] constexpr pointer get() const noexcept { return &m_value; }
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr Value operator*() const noexcept { return *get(); }

		private:
			Value m_value;
		};

		template<typename Value, typename Container, bool = std::bidirectional_iterator<std::ranges::iterator_t<Container>>>
		struct type_info_view_defs
		{
			typedef Value value_type;
			typedef type_info_ptr<Value> pointer;
			typedef type_info_ptr<Value> const_pointer;
			typedef typename Container::difference_type difference_type;
			typedef typename Container::size_type size_type;

			typedef type_info_iterator<Value, std::ranges::iterator_t<Container>> iterator;
			typedef type_info_iterator<Value, std::ranges::iterator_t<Container>> const_iterator;
		};
		template<typename Value, typename Container>
		struct type_info_view_defs<Value, Container, true> : type_info_view_defs<Value, Container, false>
		{
			using typename type_info_view_defs<Value, Container, false>::iterator;
			using typename type_info_view_defs<Value, Container, false>::const_iterator;

			typedef std::reverse_iterator<iterator> reverse_iterator;
			typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		};
	}	 // namespace detail

	/** @brief Helper type used to check if a type has been exported via `SEK_EXTERN_TYPE_INFO`. */
	template<typename T>
	struct is_exported_type : std::false_type
	{
	};
	/** @brief Alias for `is_exported_type<T>::value`. */
	template<typename T>
	constexpr static auto is_exported_type_v = is_exported_type<T>::value;

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

	/** @brief Structure representing information about a reflected attribute. */
	class attribute_info;
	/** @brief Structure representing information about a constant of a reflected type. */
	class constant_info;
	/** @brief Structure representing information about a conversion cast between reflected types. */
	class conversion_info;
	/** @brief Structure representing information about a constructor of a reflected type. */
	class constructor_info;
	/** @brief Structure representing information about a property of a reflected type. */
	class property_info;
	/** @brief Structure representing information about a function of a reflected type. */
	class function_info;

	/** @brief Handle to information about a reflected type. */
	class type_info
	{
		template<typename>
		friend class type_factory;
		friend class type_database;

	public:
		/** Returns type info for type `T`.
		 * @note Removes any const & volatile qualifiers and decays references.
		 * @note The returned type info is generated at compile time and may not be present within the type database. */
		template<typename T>
		[[nodiscard]] inline static type_info get() noexcept;
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

	private:
		using handle_t = detail::type_handle_t;
		using data_t = detail::type_data;

		template<typename V, typename T>
		using data_view = detail::type_info_view<V, T>;
		using attr_view = data_view<attribute_info, detail::attr_table>;
		using const_view = data_view<constant_info, detail::const_table>;
		using conv_view = data_view<conversion_info, detail::conv_table>;
		using ctor_view = data_view<constructor_info, std::vector<detail::ctor_data>>;

		static bool check_arg(const detail::func_arg &exp, any &value) noexcept;
		static auto find_overload(std::vector<detail::ctor_data> &range, std::span<any> args) noexcept;
		static auto find_overload(std::vector<detail::func_overload> &range, any &instance, std::span<any> args) noexcept;

		static std::error_code convert_args(std::span<const detail::func_arg> expected, std::span<any> args);

		constexpr type_info(handle_t handle) noexcept : type_info(handle()) {}
		constexpr explicit type_info(data_t *data) noexcept : m_data(data) {}

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
		/** Checks if the referenced type is a signed integral type or can be implicitly converted to `std::intptr_t`. */
		[[nodiscard]] constexpr bool is_signed() const noexcept { return valid() && m_data->is_signed; }
		/** Checks if the referenced type is an unsigned integral type or can be implicitly converted to `std::uintptr_t`. */
		[[nodiscard]] constexpr bool is_unsigned() const noexcept { return valid() && m_data->is_unsigned; }
		/** Checks if the referenced type is a floating-point type or can be implicitly converted to `long double`. */
		[[nodiscard]] constexpr bool is_floating() const noexcept { return valid() && m_data->is_floating; }

		/** Checks if the referenced type is an enum (as if via `std::is_enum_v`). */
		[[nodiscard]] constexpr bool is_enum() const noexcept { return valid() && m_data->enum_type.get; }
		/** Returns the referenced type of an enum (as if via `std::underlying_type_t`), or an invalid type info it the type is not an enum. */
		[[nodiscard]] constexpr type_info enum_type() const noexcept
		{
			return valid() ? m_data->enum_type() : type_info{};
		}

		/** Returns a view of attributes of the referenced type. */
		[[nodiscard]] constexpr attr_view attributes() const noexcept;
		/** Returns a view of constants of the referenced type. */
		[[nodiscard]] constexpr const_view constants() const noexcept;
		/** Returns a view of conversions of the referenced type. */
		[[nodiscard]] constexpr conv_view conversions() const noexcept;
		/** Returns a view of constructors of the referenced type. */
		[[nodiscard]] constexpr ctor_view constructors() const noexcept;

		/** Checks if the referenced type has an attribute of the specified type.
		 * @param type Type of the attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_attribute(type_info type) const noexcept;

		/** Checks if the referenced type has a constant with the specified name.
		 * @param name Name of the constant. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_constant(std::string_view name) const noexcept;
		/** Checks if the referenced type has a constant with the specified name and type.
		 * @param name Name of the constant.
		 * @param type Type of the constant. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_constant(std::string_view name, type_info type) const noexcept;

		/** Checks if the referenced type inherits another. That is, the other type is one of it's direct or inherited parents.
		 * @note Does not check if types are the same. */
		[[nodiscard]] SEK_CORE_PUBLIC bool inherits(type_info type) const noexcept;

		/** @brief Checks if the referenced type has a constructor overload that accepts the specified arguments.
		 * @param args Span of `type_info`, containing argument types of the constructor overload. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_constructor(std::span<const type_info> args) const noexcept;
		/** @copybrief has_constructor
		 * @param args Span of `any`, containing arguments of the constructor overload. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_constructor(std::span<const any> args) const noexcept;

		/** Checks if the referenced type has a defined conversion to another type.
		 * @note Does not check if types are the same. */
		[[nodiscard]] SEK_CORE_PUBLIC bool convertible_to(type_info type) const noexcept;

		/** Checks if the referenced type has any function overload with the specified name.
		 * @param name Name of the overloaded function. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_function(std::string_view name) const noexcept;
		/** @brief Checks if the referenced type has a function overload with the specified name that accepts the specified arguments.
		 * @param name Name of the overloaded function.
		 * @param args Span of `type_info`, containing argument types of the function overload. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_function(std::string_view name, std::span<const type_info> args) const noexcept;
		/** @copybrief has_function
		 * @param name Name of the overloaded function.
		 * @param args Span of `any`, containing arguments of the function overload. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_function(std::string_view name, std::span<const any> args) const noexcept;

		/** Checks if the referenced type has a property with the specified name.
		 * @param name Name of the property. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_property(std::string_view name) const noexcept;
		/** Checks if the referenced type has a property with the specified name and type.
		 * @param name Name of the property.
		 * @param type Type of the property. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_property(std::string_view name, type_info type) const noexcept;

		/** Returns value of the specified attribute.
		 * @param type Type of the attribute.
		 * @return `any`, containing the value of the attribute, or an empty `any` if the referenced type does not have such attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC any attribute(type_info type) const;

		/** Returns value of the specified constant.
		 * @param name Name of the constant.
		 * @return `any`, containing the value of the constant, or an empty `any` if the referenced type does not have such constant. */
		[[nodiscard]] SEK_CORE_PUBLIC any constant(std::string_view name) const;

		/** @brief Constructs an instance of the referenced type using the provided arguments.
		 * @param args Span of `any`, containing arguments passed to the constructor.
		 * @return `any` instance, containing the instantiated object, or an empty `any`
		 * if the specified constructor overload does not exist. */
		[[nodiscard]] SEK_CORE_PUBLIC any construct(std::span<any> args) const;
		/** @copybrief construct
		 * @param args Arguments passed to the constructor of the type.
		 * @return `any` instance, containing the instantiated object, or an empty `any`
		 * if the specified constructor overload does not exist. */
		template<typename... Args>
		[[nodiscard]] inline any construct(std::in_place_t, Args &&...args) const;

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept
		{
			/* If data is different, types might still be the same, but declared in different binaries. */
			return m_data == other.m_data || name() == other.name();
		}

		constexpr void swap(type_info &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(type_info &a, type_info &b) noexcept { a.swap(b); }

	private:
		data_t *m_data = nullptr;
	};

	namespace detail
	{
		template<typename T>
		constexpr type_handle_t::type_handle_t(type_selector_t<T>) noexcept : get(type_info::get<T>)
		{
		}
		constexpr type_info type_handle_t::operator()() const noexcept { return get(); }

		constexpr bool func_overload::operator==(const func_overload &other) const noexcept
		{
			return is_const == other.is_const && std::ranges::equal(args, other.args);
		}
		constexpr bool func_arg::operator==(const func_arg &other) const noexcept
		{
			return is_const == other.is_const && type() == other.type();
		}

		template<typename Value, typename Container>
		class type_info_view : public type_info_view_defs<Value, Container>
		{
			using defs = type_info_view_defs<Value, Container>;

			constexpr static bool is_bidirectional = std::bidirectional_iterator<typename defs::iterator>;
			constexpr static bool is_random_access = std::random_access_iterator<typename defs::iterator>;
			constexpr static bool is_sized = std::ranges::sized_range<Container>;

		public:
			constexpr type_info_view() noexcept = default;

			constexpr type_info_view(const Container &container, type_info parent) noexcept
				: m_container(&container), m_parent(parent)
			{
			}

			[[nodiscard]] constexpr auto begin() const noexcept
			{
				return m_container ? typename defs::iterator{m_container->begin(), m_parent} : typename defs::iterator{};
			}
			[[nodiscard]] constexpr auto end() const noexcept
			{
				return m_container ? typename defs::iterator{m_container->end(), m_parent} : typename defs::iterator{};
			}
			[[nodiscard]] constexpr auto cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr auto cend() const noexcept { return end(); }

			// clang-format off
			[[nodiscard]] constexpr auto rbegin() const noexcept requires is_bidirectional { return typename defs::reverse_iterator{end()}; }
			[[nodiscard]] constexpr auto rend() const noexcept requires is_bidirectional { return typename defs::reverse_iterator{begin()}; }
			[[nodiscard]] constexpr auto crbegin() const noexcept requires is_bidirectional { return rbegin(); }
			[[nodiscard]] constexpr auto crend() const noexcept requires is_bidirectional { return rend(); }
			// clang-format on

			[[nodiscard]] constexpr bool empty() const noexcept
			{
				return m_container && std::ranges::empty(m_container);
			}

			// clang-format off
			[[nodiscard]] constexpr auto size() const noexcept requires is_sized { return m_container ? std::ranges::size(*m_container) : 0; }
			// clang-format on

			// clang-format off
			[[nodiscard]] constexpr decltype(auto) front() const noexcept { return *begin(); }
			[[nodiscard]] constexpr decltype(auto) back() const noexcept requires is_bidirectional { return *std::prev(end()); }
			[[nodiscard]] constexpr decltype(auto) operator[](typename defs::size_type i) const noexcept requires is_random_access
			{
				return begin()[static_cast<typename defs::difference_type>(i)];
			}
			[[nodiscard]] decltype(auto) at(typename defs::size_type i) const requires is_random_access
			{
				if (i >= size()) [[unlikely]]
					throw std::out_of_range("View subscript is out of range");
				return operator[](i);
			}
			// clang-format on

			[[nodiscard]] constexpr bool operator==(const type_info_view &other) const noexcept
			{
				return m_container == other.m_container || std::equal(begin(), end(), other.m_first, other.m_last);
			}

		private:
			const Container *m_container = nullptr;
			type_info m_parent;
		};
		template<typename Value, typename Iter>
		class type_info_iterator
		{
			template<typename, typename>
			friend class type_info_view;

			using traits_t = std::iterator_traits<Iter>;

		public:
			typedef Value value_type;
			typedef type_info_ptr<Value> pointer;

			typedef typename traits_t::iterator_category iterator_category;
			typedef typename traits_t::difference_type difference_type;
			typedef std::size_t size_type;

		private:
			constexpr static bool is_bidirectional = std::is_base_of_v<std::bidirectional_iterator_tag, iterator_category>;
			constexpr static bool is_random_access = std::is_base_of_v<std::random_access_iterator_tag, iterator_category>;

			constexpr type_info_iterator(Iter iter, type_info parent) noexcept : m_iter(iter), m_parent(parent) {}

		public:
			constexpr type_info_iterator() noexcept = default;

			constexpr type_info_iterator operator++(int) noexcept
			{
				auto temp = *this;
				operator++();
				return temp;
			}
			constexpr type_info_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}

			// clang-format off
			constexpr type_info_iterator operator--(int) noexcept requires is_bidirectional
			{
				auto temp = *this;
				operator--();
				return temp;
			}
			constexpr type_info_iterator &operator--() noexcept requires is_bidirectional
			{
				--m_iter;
				return *this;
			}
			// clang-format on

			// clang-format off
			constexpr type_info_iterator &operator+=(difference_type n) noexcept requires is_random_access
			{
				m_iter += n;
				return *this;
			}
			constexpr type_info_iterator &operator-=(difference_type n) noexcept requires is_random_access
			{
				m_iter -= n;
				return *this;
			}

			[[nodiscard]] constexpr type_info_iterator operator+(difference_type n) const noexcept requires is_random_access
			{
				return type_info_iterator{m_iter + n};
			}
			[[nodiscard]] constexpr type_info_iterator operator-(difference_type n) const noexcept requires is_random_access
			{
				return type_info_iterator{m_iter - n};
			}
			[[nodiscard]] constexpr difference_type operator-(const type_info_iterator &other) const noexcept requires is_random_access
			{
				return m_iter - other.m_iter;
			}
			// clang-format on

			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{*m_iter, m_parent}; }
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr auto operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const type_info_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const type_info_iterator &) const noexcept = default;

		private:
			Iter m_iter;
			type_info m_parent;
		};
	}	 // namespace detail

	class attribute_info
	{
		template<typename>
		friend class detail::type_info_ptr;

		constexpr attribute_info(const std::ranges::range_value_t<detail::attr_table> &entry, type_info) noexcept
			: m_data(&entry.second)
		{
		}

	public:
		attribute_info() = delete;

		constexpr attribute_info(const attribute_info &) noexcept = default;
		constexpr attribute_info &operator=(const attribute_info &) noexcept = default;
		constexpr attribute_info(attribute_info &&) noexcept = default;
		constexpr attribute_info &operator=(attribute_info &&) noexcept = default;

		/** Returns the type of the attribute. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_data->type(); }
		/** Returns the value of the attribute. */
		[[nodiscard]] inline any value() const noexcept;

	private:
		const detail::attr_data *m_data;
	};
	class constant_info
	{
		template<typename>
		friend class detail::type_info_ptr;

		using attr_view = detail::type_info_view<attribute_info, detail::attr_table>;

		constexpr constant_info(const std::ranges::range_value_t<detail::const_table> &entry) noexcept
			: m_data(&entry.second)
		{
		}

	public:
		constant_info() = delete;

		constexpr constant_info(const constant_info &) noexcept = default;
		constexpr constant_info &operator=(const constant_info &) noexcept = default;
		constexpr constant_info(constant_info &&) noexcept = default;
		constexpr constant_info &operator=(constant_info &&) noexcept = default;

		/** Returns the name of the constant. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_data->name; }
		/** Returns the type of the constant. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_data->type(); }

		/** Returns a view of attributes of the constant. */
		[[nodiscard]] constexpr attr_view attributes() const noexcept { return attr_view{m_data->attributes, {}}; }

		/** @brief Checks if the constant has an attribute of the specified type.
		 * @param type Type of the attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_attribute(type_info type) const noexcept;
		/** @copybrief has_attribute
		 * @tparam T Type of the attribute. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept
		{
			return has_attribute(type_info::get<T>());
		}

		/** @brief Returns value of the specified attribute.
		 * @param type Type of the attribute.
		 * @return `any`, containing the value of the attribute, or an empty `any` if the constant does not have such attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC any attribute(type_info type) const;

		/** Returns the value of the constant. */
		[[nodiscard]] inline any value() const noexcept;

	private:
		const detail::const_data *m_data;
	};
	class conversion_info
	{
		template<typename>
		friend class detail::type_info_ptr;

		template<typename, typename>
		friend class detail::type_info_iterator;

		constexpr conversion_info(const std::ranges::range_value_t<detail::conv_table> &entry, type_info parent) noexcept
			: m_data(&entry.second), m_parent(parent)
		{
		}

	public:
		conversion_info() = delete;

		constexpr conversion_info(const conversion_info &) noexcept = default;
		constexpr conversion_info &operator=(const conversion_info &) noexcept = default;
		constexpr conversion_info(conversion_info &&) noexcept = default;
		constexpr conversion_info &operator=(conversion_info &&) noexcept = default;

		/** Returns the target type of the conversion. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_data->type(); }

		/** Converts the passed to the target type.
		 * @throw type_error If the type of the passed object is not the same as the parent type of the conversion. */
		[[nodiscard]] SEK_CORE_PUBLIC any convert() const;
		/** Converts the passed to the target type.
		 * @throw type_error If the type of the passed object is not the same as the parent type of the conversion. */
		[[nodiscard]] SEK_CORE_PUBLIC any convert(any &) const;

	private:
		const detail::conv_data *m_data;
		type_info m_parent;
	};

	template<typename T>
	type_info type_info::get() noexcept
	{
		static auto data = detail::type_data::make_instance<std::remove_cvref_t<T>>();
		return type_info{&data};
	}

	constexpr typename type_info::attr_view type_info::attributes() const noexcept
	{
		return valid() ? attr_view{m_data->attributes, *this} : attr_view{};
	}
	constexpr typename type_info::const_view type_info::constants() const noexcept
	{
		return valid() ? const_view{m_data->constants, *this} : const_view{};
	}
	constexpr typename type_info::conv_view type_info::conversions() const noexcept
	{
		return valid() ? conv_view{m_data->conversions, *this} : conv_view{};
	}
	constexpr typename type_info::ctor_view type_info::constructors() const noexcept
	{
		return valid() ? ctor_view{m_data->constructors, *this} : ctor_view{};
	}

	[[nodiscard]] constexpr std::size_t hash(const type_info &type) noexcept
	{
		const auto name = type.name();
		return fnv1a(name.data(), name.size());
	}

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
	extern template SEK_API_IMPORT sek::type_info sek::type_info::get<T>() noexcept;

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
#define SEK_EXPORT_TYPE_INFO(T) template SEK_API_EXPORT sek::type_info sek::type_info::get<T>() noexcept;

/* Type exports for reflection types */
SEK_EXTERN_TYPE_INFO(sek::any);
SEK_EXTERN_TYPE_INFO(sek::type_info);
