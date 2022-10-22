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
		template<typename From, typename To, typename Conv>
		constexpr type_conv type_conv::make_instance() noexcept
		{
			type_conv result;
			result.type = type_handle{type_selector<From>};
			if constexpr (std::is_invocable_r_v<To, Conv, From &>)
				result.convert = +[](any_ref ref) -> any
				{
					if (!ref.is_const())
					{
						auto &data = *static_cast<From *>(ref.data());
						return forward_any(Conv{}(data));
					}
					else
					{
						auto &data = *static_cast<const From *>(ref.cdata());
						return forward_any(Conv{}(data));
					}
				};
			else
				result.convert = +[](any_ref ref) -> any
				{
					auto &data = *static_cast<const From *>(ref.cdata());
					return forward_any(Conv{}(data));
				};
			return result;
		}
		template<typename From, typename To, typename Conv>
		constinit type_conv type_conv::instance = make_instance<From, To, Conv>();

		template<typename T, typename P>
		constexpr type_parent type_parent::make_instance() noexcept
		{
			type_parent result;
			result.type = type_handle{type_selector<P>};
			result.cast = +[](any_ref ref) -> any_ref
			{
				if (!ref.is_const())
				{
					auto &data = *static_cast<T *>(ref.data());
					return forward_any(static_cast<P &>(data));
				}
				else
				{
					auto &data = *static_cast<const T *>(ref.data());
					return forward_any(static_cast<const P &>(data));
				}
			};
			return result;
		}
		template<typename T, typename P>
		constinit type_parent type_parent::instance = make_instance<T, P>();

		template<typename T>
		type_data type_data::make_instance() noexcept
		{
			type_data result;
			result.name = type_name_v<T>;

			/* When a reflected type is reset, the type data will be in a "dirty" state, as it will still be
			 * referencing the reflected data objects. This may cause issues especially if a plugin that has
			 * originally reflected the type is unloaded. As such, type data needs to be reset to its original state. */
			result.reset = +[](type_data *ptr) -> void { *ptr = make_instance<T>(); };

			/* Initialize flags. */
			result.is_void = std::is_void_v<T>;
			result.is_empty = std::is_empty_v<T>;
			result.is_nullptr = std::same_as<T, std::nullptr_t>;

			/* Initialize default conversions. */
			if constexpr (std::is_enum_v<T>)
			{
				auto &conv = type_conv::instance<T, std::underlying_type_t<T>, default_conv<T, std::underlying_type_t<T>>>;
				result.conversions.insert(conv);
				result.enum_cast = &conv;
			}
			if constexpr (std::is_convertible_v<T, std::intptr_t>)
			{
				auto &conv = type_conv::instance<T, std::intptr_t, default_conv<T, std::intptr_t>>;
				result.conversions.insert(conv);
				result.signed_cast = &conv;
			}
			if constexpr (std::is_convertible_v<T, std::uintptr_t>)
			{
				auto &conv = type_conv::instance<T, std::uintptr_t, default_conv<T, std::uintptr_t>>;
				result.conversions.insert(conv);
				result.unsigned_cast = &conv;
			}
			if constexpr (std::is_convertible_v<T, long double>)
			{
				auto &conv = type_conv::instance<T, long double, default_conv<T, long double>>;
				result.conversions.insert(conv);
				result.floating_cast = &conv;
			}

			/* Initialize type-erased operations. */
			result.any_funcs = any_funcs_t::make_instance<T>();
			if constexpr (tuple_like<T>) result.tuple_data = &tuple_type_data::instance<T>;
			if constexpr (std::ranges::range<T>)
			{
				result.range_data = &range_type_data::instance<T>;
				if constexpr (table_range_type<T>) result.table_data = &table_type_data::instance<T>;
			}
			if constexpr (string_like_type<T>) result.string_data = &string_type_data::instance<T>;

			return result;
		}
		template<typename T>
		type_data *type_data::instance() noexcept
		{
			static auto value = make_instance<T>();
			return &value;
		}

		/* Custom view, as CLang has issues with `std::ranges::subrange` at this time. */
		template<typename Iter>
		class type_info_view
		{
		public:
			typedef typename Iter::value_type value_type;
			typedef typename Iter::pointer pointer;
			typedef typename Iter::const_pointer const_pointer;
			typedef typename Iter::reference reference;
			typedef typename Iter::const_reference const_reference;
			typedef typename Iter::difference_type difference_type;
			typedef typename Iter::size_type size_type;

			typedef Iter iterator;
			typedef Iter const_iterator;

		public:
			constexpr type_info_view() noexcept = default;
			constexpr type_info_view(iterator first, iterator last) noexcept : m_first(first), m_last(last) {}

			[[nodiscard]] constexpr iterator begin() const noexcept { return m_first; }
			[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
			[[nodiscard]] constexpr iterator end() const noexcept { return m_last; }
			[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }

			// clang-format off
			[[nodiscard]] constexpr reference front() const noexcept { return *begin(); }
			[[nodiscard]] constexpr reference back() const noexcept requires std::bidirectional_iterator<Iter>
			{
				return *std::prev(end());
			}
			[[nodiscard]] constexpr reference at(size_type i) const noexcept requires std::random_access_iterator<Iter>
			{
				return begin()[static_cast<difference_type>(i)];
			}
			[[nodiscard]] constexpr reference operator[](size_type i) const noexcept requires std::random_access_iterator<Iter>
			{
				return at(i);
			}
			// clang-format on

			[[nodiscard]] constexpr bool empty() const noexcept { return begin() == end(); }
			[[nodiscard]] constexpr size_type size() const noexcept
			{
				return static_cast<size_type>(std::distance(begin(), end()));
			}

			[[nodiscard]] constexpr bool operator==(const type_info_view &other) const noexcept
			{
				return std::equal(m_first, m_last, other.m_first, other.m_last);
			}

		private:
			Iter m_first = {};
			Iter m_last = {};
		};

		template<typename Value, typename Iter>
		class type_info_iterator
		{
			friend class sek::type_info;

		public:
			typedef Value value_type;
			typedef const Value *pointer;
			typedef const Value *const_pointer;
			typedef const Value &reference;
			typedef const Value &const_reference;
			typedef std::ptrdiff_t difference_type;
			typedef std::size_t size_type;
			typedef std::forward_iterator_tag iterator_category;

		private:
			constexpr explicit type_info_iterator(Iter iter) noexcept : m_iter(iter) {}

		public:
			constexpr type_info_iterator() noexcept = default;

			constexpr type_info_iterator &operator++() noexcept
			{
				m_iter++;
				return *this;
			}
			constexpr type_info_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++m_iter;
				return temp;
			}

			[[nodiscard]] constexpr pointer get() const noexcept { return static_cast<pointer>(m_iter.get()); }
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr bool operator==(const type_info_iterator &) const noexcept = default;

		private:
			Iter m_iter = {};
		};
	}	 // namespace detail

	/** @brief Structure representing a type cast of a reflected type. */
	class type_conversion : detail::type_conv
	{
		template<typename, typename>
		friend class detail::type_info_iterator;

		using base_t = detail::type_conv;

	public:
		type_conversion() = delete;
		type_conversion(const type_conversion &) = delete;
		type_conversion &operator=(const type_conversion &) = delete;
		type_conversion(type_conversion &&) = delete;
		type_conversion &operator=(type_conversion &&) = delete;

		/** Returns type info of the converted-to type. */
		[[nodiscard]] constexpr type_info type() const noexcept;
		/** @copydoc type */
		[[nodiscard]] constexpr operator type_info() const noexcept;

		/** Preforms a value-cast (as-if via `static_cast`) of the passed object to the converted-to type.
		 * @return `any`, referencing the converted object. */
		[[nodiscard]] any convert(any value) const { return convert(value); }
		/** @copydoc convert */
		[[nodiscard]] any convert(any_ref value) const { return base_t::convert(value); }

		[[nodiscard]] constexpr bool operator==(const type_info &) const noexcept;
		[[nodiscard]] constexpr bool operator==(const type_conversion &) const noexcept;
	};
	/** @brief Structure representing a parent-child relationship of a reflected type. */
	class type_parent : detail::type_parent
	{
		template<typename, typename>
		friend class detail::type_info_iterator;

		using base_t = detail::type_parent;

	public:
		type_parent() = delete;
		type_parent(const type_parent &) = delete;
		type_parent &operator=(const type_parent &) = delete;
		type_parent(type_parent &&) = delete;
		type_parent &operator=(type_parent &&) = delete;

		/** Returns type info of the parent type. */
		[[nodiscard]] constexpr type_info type() const noexcept;
		/** @copydoc type */
		[[nodiscard]] constexpr operator type_info() const noexcept;

		/** Casts the passed object reference to a reference of parent type.
		 * @return `any_ref`, referencing the type-cast object. */
		[[nodiscard]] any_ref cast(any_ref value) const { return base_t::cast(value); }

		[[nodiscard]] constexpr bool operator==(const type_info &) const noexcept;
		[[nodiscard]] constexpr bool operator==(const type_parent &) const noexcept;
	};
	/** @brief Structure representing an attribute of a reflected type. */
	class type_attribute : detail::type_attr
	{
		template<typename, typename>
		friend class detail::type_info_iterator;

		using base_t = detail::type_attr;

	public:
		type_attribute() = delete;
		type_attribute(const type_attribute &) = delete;
		type_attribute &operator=(const type_attribute &) = delete;
		type_attribute(type_attribute &&) = delete;
		type_attribute &operator=(type_attribute &&) = delete;

		/** Returns type info of the attribute. */
		[[nodiscard]] constexpr type_info type() const noexcept;
		/** Returns `any_ref` reference to the attribute. */
		[[nodiscard]] any_ref get() const { return base_t::get(this); }
	};
	/** @brief Structure representing a constant of a reflected type. */
	class type_constant : detail::type_const
	{
		template<typename, typename>
		friend class detail::type_info_iterator;

		using base_t = detail::type_const;

	public:
		type_constant() = delete;
		type_constant(const type_constant &) = delete;
		type_constant &operator=(const type_constant &) = delete;
		type_constant(type_constant &&) = delete;
		type_constant &operator=(type_constant &&) = delete;

		/** Returns type info of the constant. */
		[[nodiscard]] constexpr type_info type() const noexcept;
		/** Returns the name of the constant. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return base_t::name; }

		/** Returns an instance of the attribute. */
		[[nodiscard]] any get() const { return base_t::get(); }
	};

	/** @brief Handle to information about a reflected type. */
	class type_info
	{
		friend struct detail::type_handle;

		friend class type_conversion;
		friend class type_parent;
		friend class type_attribute;
		friend class type_constant;

		friend class any;
		friend class any_ref;
		friend class any_range;
		friend class any_table;
		friend class any_tuple;
		friend class any_string;
		template<typename>
		friend class type_factory;
		friend class type_database;

		using data_t = detail::type_data;
		using handle_t = detail::type_handle;

		template<typename I>
		using info_view = detail::type_info_view<I>;
		template<typename V, typename I>
		using info_iterator = detail::type_info_iterator<V, I>;

		// clang-format off
		using conversion_iterator = info_iterator<type_conversion, typename detail::type_data_list<detail::type_conv>::iterator>;
		using parent_iterator = info_iterator<type_parent, typename detail::type_data_list<detail::type_parent>::iterator>;
		using attribute_iterator = info_iterator<type_attribute, typename detail::type_data_list<detail::type_attr>::iterator>;
		using constant_iterator = info_iterator<type_constant, typename detail::type_data_list<detail::type_const>::iterator>;
		// clang-format on

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
		[[nodiscard]] constexpr bool is_enum() const noexcept { return valid() && m_data->enum_cast; }
		/** Checks if the referenced type is a signed integral type or can be converted to `std::intptr_t`. */
		[[nodiscard]] constexpr bool is_signed() const noexcept { return valid() && m_data->signed_cast; }
		/** Checks if the referenced type is an unsigned integral type or can be converted to `std::uintptr_t`. */
		[[nodiscard]] constexpr bool is_unsigned() const noexcept { return valid() && m_data->unsigned_cast; }
		/** Checks if the referenced type is a floating-point type or can be converted to `long double`. */
		[[nodiscard]] constexpr bool is_floating() const noexcept { return valid() && m_data->floating_cast; }

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
			return valid() ? type_info{m_data->enum_cast->type} : type_info{};
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

		/** Returns a range of type conversions of the referenced type. */
		[[nodiscard]] constexpr info_view<conversion_iterator> conversions() const noexcept
		{
			return {conversion_iterator{m_data->conversions.begin()}, conversion_iterator{m_data->conversions.end()}};
		}
		/** Returns a range of parent relationships of the referenced type. */
		[[nodiscard]] constexpr info_view<parent_iterator> parents() const noexcept
		{
			return {parent_iterator{m_data->parents.begin()}, parent_iterator{m_data->parents.end()}};
		}
		/** Returns a range of attributes of the referenced type. */
		[[nodiscard]] constexpr info_view<attribute_iterator> attributes() const noexcept
		{
			return {attribute_iterator{m_data->attributes.begin()}, attribute_iterator{m_data->attributes.end()}};
		}
		/** Returns a range of constants of the referenced type. */
		[[nodiscard]] constexpr info_view<constant_iterator> constants() const noexcept
		{
			return {constant_iterator{m_data->constants.begin()}, constant_iterator{m_data->constants.end()}};
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

	constexpr type_info type_parent::type() const noexcept { return type_info{base_t::type}; }
	constexpr type_parent::operator type_info() const noexcept { return type(); }

	constexpr bool type_parent::operator==(const type_info &info) const noexcept { return type() == info; }
	constexpr bool type_parent::operator==(const type_parent &other) const noexcept { return type() == other.type(); }

	constexpr type_info type_conversion::type() const noexcept { return type_info{base_t::type}; }
	constexpr type_conversion::operator type_info() const noexcept { return type(); }

	constexpr bool type_conversion::operator==(const type_info &info) const noexcept { return type() == info; }
	constexpr bool type_conversion::operator==(const type_conversion &other) const noexcept
	{
		return type() == other.type();
	}

	constexpr type_info type_attribute::type() const noexcept { return type_info{base_t::type}; }
	constexpr type_info type_constant::type() const noexcept { return type_info{base_t::type}; }

	template<typename T>
	constexpr type_info type_factory<T>::type() const noexcept
	{
		return type_info{m_data};
	}

	any::any(type_info type, void *ptr) noexcept
	{
		m_storage = base_t::storage_t{ptr, false};
		m_type = type.m_data;
	}
	any::any(type_info type, const void *ptr) noexcept
	{
		m_storage = base_t::storage_t{ptr, true};
		m_type = type.m_data;
	}

	constexpr type_info any::type() const noexcept { return type_info{m_type}; }
	constexpr type_info any_ref::type() const noexcept { return type_info{m_type}; }

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
	struct type_name<any_ref>
	{
		constexpr static std::string_view value = "sek::any_ref";
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
SEK_EXTERN_TYPE_INFO(sek::any_ref);
SEK_EXTERN_TYPE_INFO(sek::type_info);
