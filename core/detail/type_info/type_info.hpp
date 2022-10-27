/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

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
		template<typename Value, typename Iter>
		class type_info_iterator;

		template<typename Value, typename Container, bool = std::bidirectional_iterator<std::ranges::iterator_t<Container>>>
		struct type_info_view_defs
		{
			typedef Value value_type;
			typedef const Value *pointer;
			typedef const Value *const_pointer;
			typedef const Value &reference;
			typedef const Value &const_reference;
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

		/* Custom view, as CLang has issues with `std::ranges::subrange` at this time. */
		template<typename Value, typename Container>
		class type_info_view : public type_info_view_defs<Value, Container>
		{
			using defs = type_info_view_defs<Value, Container>;

			constexpr static bool is_bidirectional = std::bidirectional_iterator<typename defs::iterator>;
			constexpr static bool is_random_access = std::random_access_iterator<typename defs::iterator>;
			constexpr static bool is_sized = std::ranges::sized_range<Container>;

		public:
			constexpr type_info_view() noexcept = default;

			constexpr explicit type_info_view(const Container &container) noexcept : m_container(&container) {}

			[[nodiscard]] constexpr auto begin() const noexcept
			{
				return m_container ? typename defs::iterator{m_container->begin()} : typename defs::iterator{};
			}
			[[nodiscard]] constexpr auto end() const noexcept
			{
				return m_container ? typename defs::iterator{m_container->end()} : typename defs::iterator{};
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
		};

		template<typename Value, typename Iter>
		class type_info_iterator
		{
			template<typename, typename>
			friend class type_info_view;

			using traits_t = std::iterator_traits<Iter>;

		public:
			typedef Value value_type;
			typedef const Value *pointer;
			typedef const Value &reference;

			typedef typename traits_t::iterator_category iterator_category;
			typedef typename traits_t::difference_type difference_type;
			typedef std::size_t size_type;

		private:
			constexpr static bool is_bidirectional = std::is_base_of_v<std::bidirectional_iterator_tag, iterator_category>;
			constexpr static bool is_random_access = std::is_base_of_v<std::random_access_iterator_tag, iterator_category>;

			constexpr explicit type_info_iterator(Iter iter) noexcept : m_iter(iter) {}

		public:
			constexpr type_info_iterator() noexcept = default;

			constexpr type_info_iterator operator++(int) noexcept
			{
				auto tmp = *this;
				operator++();
				return tmp;
			}
			constexpr type_info_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}

			// clang-format off
			constexpr type_info_iterator operator--(int) noexcept requires is_bidirectional
			{
				auto tmp = *this;
				operator--();
				return tmp;
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

			[[nodiscard]] constexpr pointer get() const noexcept
			{
				return static_cast<pointer>(std::to_address(m_iter));
			}
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const type_info_iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const type_info_iterator &) const noexcept = default;

		private:
			Iter m_iter;
		};
	}

	/** @brief Structure used to describe a reflected attribute. */
	class attribute_info;
	/** @brief Structure used to describe a reflected type constant. */
	class constant_info;

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

		friend class attribute_info;
		friend class constant_info;

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

	private:
		using data_t = detail::type_data;
		using handle_t = detail::type_handle;

		template<typename V, typename T>
		using data_view = detail::type_info_view<V, T>;
		using attr_view = data_view<attribute_info, detail::attr_table>;
		using const_view = data_view<constant_info, detail::const_table>;

		template<typename T>
		[[nodiscard]] constexpr static handle_t handle() noexcept
		{
			return handle_t{type_selector<std::remove_cvref_t<T>>};
		}

		static auto find_overload(auto &range, std::span<type_info> args);
		static auto find_overload(auto &range, std::span<any> args);

		constexpr explicit type_info(data_t *data) noexcept : m_data(data) {}
		constexpr explicit type_info(handle_t handle) noexcept : m_data(handle.get ? handle.get() : nullptr) {}

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

		/** Returns a view of attributes of the referenced type. */
		[[nodiscard]] constexpr attr_view attributes() const noexcept
		{
			if (!valid()) [[unlikely]]
				return {};
			return attr_view{m_data->attributes};
		}

		/** @brief Checks if the referenced type has an attribute of the specified type.
		 * @param type Type of the attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_attribute(type_info type) const noexcept;
		/** @copybrief has_attribute
		 * @tparam T Type of the attribute. */
		template<typename T>
		[[nodiscard]] bool has_attribute() const noexcept
		{
			return has_attribute(get<T>());
		}

		/** @brief Returns value of the specified attribute.
		 * @param type Type of the attribute.
		 * @return `any`, containing the value of the attribute, or an empty `any` if the referenced type does not have such attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC any attribute(type_info type) const;
		/** @copybrief attribute
		 * @tparam T Type of the attribute.
		 * @return `any`, containing the value of the attribute, or an empty `any` if the referenced type does not have such attribute. */
		template<typename T>
		[[nodiscard]] any attribute() const
		{
			return attribute(get<T>());
		}

		/** Returns a view of constants of the referenced type. */
		[[nodiscard]] constexpr const_view constants() const noexcept
		{
			if (!valid()) [[unlikely]]
				return {};
			return const_view{m_data->constants};
		}

		/** Checks if the referenced type has a constant with the specified name.
		 * @param name Name of the constant. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_constant(std::string_view name) const noexcept;
		/** @brief Checks if the referenced type has a constant with the specified name and type.
		 * @param name Name of the constant.
		 * @param type Type of the constant. */
		[[nodiscard]] SEK_CORE_PUBLIC bool has_constant(std::string_view name, type_info type) const noexcept;
		/** @copybrief has_constant
		 * @tparam T Type of the constant.
		 * @param name Name of the constant. */
		template<typename T>
		[[nodiscard]] bool has_constant(std::string_view name) const noexcept
		{
			return has_constant(name, get<T>());
		}

		/** @brief Returns value of the specified constant.
		 * @param name Name of the constant.
		 * @return `any`, containing the value of the constant, or an empty `any` if the referenced type does not have such constant. */
		[[nodiscard]] SEK_CORE_PUBLIC any constant(std::string_view name) const;

		/** Checks if the referenced type inherits another. That is, the other type is one of it's direct or inherited parents.
		 * @note Does not check if types are the same. */
		[[nodiscard]] SEK_CORE_PUBLIC bool inherits(type_info type) const noexcept;
		/** @copydoc inherits */
		template<typename T>
		[[nodiscard]] bool inherits() const noexcept
		{
			return inherits(get<T>());
		}

		/** Constructs an instance of the referenced type using the provided arguments.
		 * @param args Span of `any`, containing arguments passed to the constructor.
		 * @return `any` instance, containing the instantiated object, or an empty `any`
		 * if the specified constructor overload does not exist. */
		[[nodiscard]] SEK_CORE_PUBLIC any construct(std::span<any> args) const;
		/** Constructs an instance of the referenced type using the provided arguments.
		 * @param args Arguments passed to the constructor of the type.
		 * @return `any` instance, containing the instantiated object, or an empty `any`
		 * if the specified constructor overload does not exist. */
		template<typename... Args>
		[[nodiscard]] any construct(Args &&...args) const
		{
			std::array<any, sizeof...(Args)> local_args = {forward_any(std::forward<Args>(args))...};
			return construct(local_args);
		}

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

	[[nodiscard]] constexpr hash_t hash(const type_info &type) noexcept
	{
		const auto name = type.name();
		return fnv1a(name.data(), name.size());
	}

	class attribute_info : detail::attr_data
	{
		template<typename, typename>
		friend class detail::type_info_iterator;

		using base_t = detail::attr_data;

	public:
		attribute_info() = delete;
		attribute_info(const attribute_info &) = delete;
		attribute_info &operator=(const attribute_info &) = delete;
		attribute_info(attribute_info &&) = delete;
		attribute_info &operator=(attribute_info &&) = delete;

		/** Returns the type of the attribute. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{base_t::type}; }
		/** Returns the value of the attribute. */
		[[nodiscard]] any value() const noexcept { return base_t::get(); }
	};
	class constant_info : detail::const_data
	{
		template<typename, typename>
		friend class detail::type_info_iterator;

		using attr_view = detail::type_info_view<attribute_info, detail::attr_table>;
		using base_t = detail::const_data;

	public:
		constant_info() = delete;
		constant_info(const constant_info &) = delete;
		constant_info &operator=(const constant_info &) = delete;
		constant_info(constant_info &&) = delete;
		constant_info &operator=(constant_info &&) = delete;

		/** Returns the name of the constant. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return base_t::name; }
		/** Returns the type of the constant. */
		[[nodiscard]] constexpr type_info type() const noexcept { return type_info{base_t::type}; }

		/** Returns a view of attributes of the constant. */
		[[nodiscard]] constexpr attr_view attributes() const noexcept { return attr_view{base_t::attributes}; }

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
		/** @copybrief attribute
		 * @tparam T Type of the attribute.
		 * @return `any`, containing the value of the attribute, or an empty `any` if the constant does not have such attribute. */
		template<typename T>
		[[nodiscard]] any attribute() const
		{
			return attribute(type_info::get<T>());
		}

		/** Returns the value of the constant. */
		[[nodiscard]] any value() const noexcept { return base_t::get(); }
	};

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
