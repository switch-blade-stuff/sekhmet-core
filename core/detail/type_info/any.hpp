/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include <bit>
#include <memory>
#include <utility>

#include "../../expected.hpp"
#include "type_info.hpp"

namespace sek
{
	/** @brief Type-erased container of objects. */
	class any
	{
		friend struct detail::any_vtable;
		friend struct detail::ctor_data;

		friend class any_tuple;
		friend class any_range;
		friend class any_table;
		friend class any_string;

		friend bool SEK_CORE_PUBLIC operator==(const any &a, const any &b) noexcept;
		friend bool SEK_CORE_PUBLIC operator<(const any &a, const any &b) noexcept;
		friend bool SEK_CORE_PUBLIC operator<=(const any &a, const any &b) noexcept;
		friend bool SEK_CORE_PUBLIC operator>(const any &a, const any &b) noexcept;
		friend bool SEK_CORE_PUBLIC operator>=(const any &a, const any &b) noexcept;

		struct storage_t
		{
			using data_t = std::conditional_t<sizeof(std::uintptr_t) < sizeof(std::uintmax_t), std::uintmax_t, std::uintptr_t>;

			template<typename T, typename U = std::remove_cvref_t<T>>
			constexpr static bool local_candidate = sizeof(U) <= sizeof(data_t) && std::is_trivially_copyable_v<U>;

			constexpr storage_t() noexcept = default;
			constexpr storage_t(const storage_t &) noexcept = default;
			constexpr storage_t &operator=(const storage_t &) noexcept = default;

			constexpr storage_t(storage_t &&other) noexcept
				: data(std::exchange(other.data, data_t{})),
				  is_local(std::exchange(other.is_local, false)),
				  is_const(std::exchange(other.is_const, false))
			{
			}
			constexpr storage_t &operator=(storage_t &&other) noexcept
			{
				swap(other);
				return *this;
			}

			// clang-format off
			template<typename T>
			storage_t(T *ptr, bool is_const) noexcept : data(std::bit_cast<data_t>(ptr)), is_ref(true), is_const(is_const)
			{
			}
			template<typename T>
			storage_t(T *ptr) noexcept : storage_t(ptr, std::is_const_v<T>) {}

			template<typename T, typename U>
			storage_t(std::in_place_type_t<T>, U &ref) requires std::is_lvalue_reference_v<T> : storage_t(std::addressof(ref)) {}
			// clang-format on

			template<typename T, typename... Args>
			storage_t(std::in_place_type_t<T>, Args &&...args)
			{
				init<T>(std::construct_at<T, Args...>, std::forward<Args>(args)...);
				init_flags<T>();
			}
			storage_t(std::in_place_type_t<void>, auto &&...) {}

			template<typename T>
			constexpr void init_flags() noexcept
			{
				is_ref = std::is_lvalue_reference_v<T>;
				is_local = local_candidate<T>;
				is_const = std::is_const_v<std::remove_reference_t<T>>;
			}

			// clang-format off
			template<typename T, typename F, typename... Args>
			void init(F &&ctor, Args &&...args)
			{
				constexpr auto align = std::align_val_t{alignof(T)};
				constexpr auto size = sizeof(T);

				auto *ptr = static_cast<T *>(::operator new(size, align));
				std::invoke(std::forward<F>(ctor), ptr, std::forward<Args>(args)...);

				deleter = +[](data_t ptr) { ::operator delete(std::bit_cast<void *>(ptr), size, align); };
				data = std::bit_cast<data_t>(ptr);
			}
			template<typename T, typename F, typename... Args>
			void init(F &&ctor, Args &&...args) requires local_candidate<T>
			{
				const auto ptr = std::bit_cast<T *>(&data);
				std::invoke(std::forward<F>(ctor), ptr, std::forward<Args>(args)...);
			}
			// clang-format on

			void do_delete()
			{
				if (deleter != nullptr) [[likely]]
					deleter(data);
			}

			[[nodiscard]] constexpr void *get() noexcept
			{
				if (is_local)
					return std::bit_cast<void *>(&data);
				else
					return std::bit_cast<void *>(data);
			}
			[[nodiscard]] constexpr const void *get() const noexcept
			{
				if (is_local)
					return std::bit_cast<const void *>(&data);
				else
					return std::bit_cast<const void *>(data);
			}

			template<typename T>
			[[nodiscard]] constexpr auto *get() noexcept
			{
				return static_cast<T *>(get());
			}
			template<typename T>
			[[nodiscard]] constexpr auto *get() const noexcept
			{
				return static_cast<std::add_const_t<T> *>(get());
			}

			[[nodiscard]] storage_t ref() noexcept { return {get(), is_const}; }
			[[nodiscard]] storage_t ref() const noexcept { return {get(), true}; }

			constexpr void swap(storage_t &other) noexcept
			{
				std::swap(deleter, other.deleter);
				std::swap(data, other.data);
				std::swap(is_ref, other.is_ref);
				std::swap(is_local, other.is_local);
				std::swap(is_const, other.is_const);
			}

			void (*deleter)(data_t) = nullptr;
			data_t data = {};
			bool is_ref = false;
			bool is_local = false;
			bool is_const = false;
		};

		template<typename T, typename F, typename... Args>
		[[nodiscard]] static any construct_with(F &&ctor, Args &&...args)
		{
			storage_t storage;
			storage.template init<T>(std::forward<F>(ctor), std::forward<Args>(args)...);
			storage.template init_flags<T>();
			return any{detail::type_handle{type_selector<T>}.get(), std::move(storage)};
		}

		[[noreturn]] static SEK_CORE_PUBLIC void throw_bad_cast(type_info from, type_info to);
		[[noreturn]] static SEK_CORE_PUBLIC void throw_bad_conv(type_info from, type_info to);

		constexpr any(detail::type_data *type, storage_t &&storage) : m_type(type), m_storage(std::move(storage)) {}

	public:
		/** Initializes an empty `any`. */
		constexpr any() noexcept = default;
		~any() { destroy(); }

		// clang-format off
		/** Constructs an instance of type `T` in-place.
		 * @param args Arguments passed to constructor of `T`. */
		template<typename T, typename... Args>
		any(std::in_place_type_t<T> p, Args &&...args)
			: m_type(detail::type_handle{type_selector<std::remove_cvref_t<T>>}.get()),
			  m_storage(p, std::forward<Args>(args)...) {}
		/** @copydoc any
		 * @param il Initializer list passed to constructor of `T`. */
		template<typename T, typename U, typename... Args>
		any(std::in_place_type_t<T> p, std::initializer_list<U> il, Args &&...args)
			: m_type(detail::type_handle{type_selector<std::remove_cvref_t<T>>}.get()),
			  m_storage(p, il, std::forward<Args>(args)...) {}
		/** Initializes `any` with a reference of type `T`.
		 * @param ref Reference to an external object. */
		template<typename T, typename U>
		any(std::in_place_type_t<T> p, U &ref) requires std::is_lvalue_reference_v<T>
			: m_type(detail::type_handle{type_selector<std::remove_cvref_t<T>>}.get()),
			  m_storage(p, ref) {}
		// clang-format on

		// clang-format off
		/** Initializes `any` from the specified value. If `T` an lvalue, initializes an `any` with a reference to `value`.
		 * @param value Value to use for initialization.
		 * @note Equivalent to `any(std::in_place_type<T>, std::forward<T>(value))`. */
		template<typename T>
		any(T &&value) requires(!std::same_as<std::decay_t<T>, any>) : any(std::in_place_type<T>, std::forward<T>(value)) {}
		// clang-format on

		constexpr any(any &&other) noexcept { move_init(std::move(other)); }
		constexpr any &operator=(any &&other) noexcept
		{
			move_assign(std::move(other));
			return *this;
		}

		/** Copy-constructs the managed object of `other`.
		 * @throw type_error If the underlying type is not copy-constructable. */
		any(const any &other) { copy_init(other); }
		/** Copy-assigns the managed object of `other`.
		 * @throw type_error If the underlying type is not copy-assignable or copy-constructable. */
		any &operator=(const any &other)
		{
			if (this != &other) [[likely]]
				copy_assign(other);
			return *this;
		}

		/** Initializes an `any` reference to the specified object.
		 * @param type Type of the pointed-to object.
		 * @param ptr Pointer to the object.
		 * @warning If the pointed-to object's type is not the same as `type`, results in undefined behavior. */
		inline any(type_info type, void *ptr) noexcept;
		/** @copydoc any */
		inline any(type_info type, const void *ptr) noexcept;

		/** Checks if the `any` instance is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_type == nullptr; }

		/** Returns type of the managed object, or an invalid `type_info` if empty. */
		[[nodiscard]] constexpr type_info type() const noexcept;

		/** Checks if the `any` instance contains a reference to an object. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return m_storage.is_ref; }
		/** Checks if the `any` instance manages a const-qualified object. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_storage.is_const; }

		/** Releases the managed object. */
		void reset()
		{
			destroy();
			m_type = nullptr;
			m_storage = {};
		}

		/** Returns raw pointer to the managed object's data.
		 * @note If the managed object is const-qualified, returns nullptr. */
		[[nodiscard]] void *data() noexcept
		{
			if (is_const()) [[unlikely]]
				return nullptr;
			return m_storage.get();
		}
		/** Returns raw const pointer to the managed object's data. */
		[[nodiscard]] const void *cdata() const noexcept { return m_storage.get(); }
		/** @copydoc cdata */
		[[nodiscard]] const void *data() const noexcept { return cdata(); }

		/** Returns a pointer to the managed object's data.
		 * @note If the managed object is const-qualified, while `T` is not, or is of a different type than `T`, returns nullptr. */
		template<typename T>
		[[nodiscard]] T *get() noexcept;
		/** Returns a const pointer to the managed object's data.
		 * @note If the managed object is of different type than `T`, returns nullptr. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *get() const noexcept;

		/** Converts reference to the managed object type to the specified parent type.
		 * @return Reference to the managed object, converted to the specified type, or an empty `any` instance if the
		 * specified type is not same as the managed object's type or a parent of it.
		 * @note If the type of the managed object is the same as specified, equivalent to `ref()`. */
		[[nodiscard]] SEK_CORE_PUBLIC any as(type_info type);
		/** @copydoc as */
		[[nodiscard]] SEK_CORE_PUBLIC any as(type_info type) const;

		// clang-format off
		/** Converts reference to the managed object type to the specified type.
		 * @return Reference to the managed object, converted to `T`.
		 * @throw type_error If the specified type is not same as the managed object's type or a parent of it.
		 * @note If the type of the managed object is the same as specified, returns reference to the managed object. */
		template<typename T, typename U = std::remove_reference_t<T>>
		[[nodiscard]] inline U &as() requires std::is_lvalue_reference_v<T>;
		/** @copydoc as */
		template<typename T, typename U = std::remove_reference_t<T>>
		[[nodiscard]] inline std::add_const_t<U> &as() const requires std::is_lvalue_reference_v<T>;
		// clang-format on

		// clang-format off
		/** Converts pointer to the managed object type to the specified type.
		 * @return Pointer to the managed object, converted to `T`, or `nullptr` if the
		 * specified type is not same as the managed object's type or a parent of it.
		 * @note If the type of the managed object is the same as specified, returns pointer to the managed object. */
		template<typename T, typename U = std::remove_pointer_t<T>>
		[[nodiscard]] inline U *as() requires std::is_pointer_v<T>;
		/** @copydoc as */
		template<typename T, typename U = std::remove_pointer_t<T>>
		[[nodiscard]] inline std::add_const_t<U> *as() const requires std::is_pointer_v<T>;
		// clang-format on

		/** Converts the managed object to the specified type as-if via `static_cast<To>(value)`.
		 * @return Result of the conversion, or an empty `any` if the types are not convertible. */
		[[nodiscard]] SEK_CORE_PUBLIC any conv(type_info type) const;

		/** Creates an `any` instance referencing the managed object. */
		[[nodiscard]] any ref() noexcept { return any{m_type, m_storage.ref()}; }
		/** Creates an `any` instance referencing the managed object via const-reference. */
		[[nodiscard]] any ref() const noexcept { return any{m_type, m_storage.ref()}; }
		/** @copydoc ref */
		[[nodiscard]] any cref() const noexcept { return ref(); }

		constexpr void swap(any &other) noexcept
		{
			std::swap(m_type, other.m_type);
			m_storage.swap(other.m_storage);
		}
		friend constexpr void swap(any &a, any &b) noexcept { a.swap(b); }

	private:
		SEK_CORE_PUBLIC void destroy();
		SEK_CORE_PUBLIC void copy_init(const any &other);
		SEK_CORE_PUBLIC void copy_assign(const any &other);
		constexpr void move_init(any &&other) noexcept
		{
			m_type = std::exchange(other.m_type, nullptr);
			m_storage = std::move(other.m_storage);
		}
		constexpr void move_assign(any &&other) noexcept
		{
			std::swap(m_type, other.m_type);
			m_storage = std::move(other.m_storage);
		}

		[[nodiscard]] any as_impl(type_info type, bool const_ref) const;

		detail::type_data *m_type = nullptr;
		storage_t m_storage = {};
	};

	template<typename T>
	T *any::get() noexcept
	{
		if (type() != type_info::get<T>()) [[unlikely]]
			return nullptr;

		if constexpr (std::is_const_v<T>)
			return static_cast<T *>(cdata());
		else
			return static_cast<T *>(data());
	}
	template<typename T>
	std::add_const_t<T> *any::get() const noexcept
	{
		if (type() != type_info::get<T>()) [[unlikely]]
			return nullptr;
		return static_cast<std::add_const_t<T> *>(data());
	}

	// clang-format off
	template<typename T, typename U>
	U &any::as() requires std::is_lvalue_reference_v<T>
	{
		const auto to_type = type_info::get<T>();
		const auto result = as(to_type);
		if (result.empty()) [[unlikely]]
			throw_bad_cast(type(), to_type);
		return *static_cast<U *>(result.data());
	}
	template<typename T, typename U>
	std::add_const_t<U> &any::as() const requires std::is_lvalue_reference_v<T>
	{
		const auto to_type = type_info::get<T>();
		const auto result = as(to_type);
		if (result.empty()) [[unlikely]]
			throw_bad_cast(type(), to_type);
		return *static_cast<U *>(result.data());
	}
	// clang-format on

	// clang-format off
	template<typename T, typename U>
	U *any::as() requires std::is_pointer_v<T> { return static_cast<U *>(as(type_info::get<T>()).data()); }
	template<typename T, typename U>
	std::add_const_t<U> *any::as() const requires std::is_pointer_v<T> { return static_cast<const U *>(as(type_info::get<T>()).data()); }
	// clang-format on

	/** If `value` is a non-rvalue instance of `any`, creates an `any` instance referencing the managed object.
	 * Otherwise, equivalent to `any(std::forward<T>(value))`. */
	template<typename T>
	[[nodiscard]] any forward_any(T &&value)
	{
		if constexpr (!(std::is_lvalue_reference_v<T> && std::same_as<std::decay_t<T>, any>) )
			return any{std::forward<T>(value)};
		else
			return value.ref();
	}
	/** @brief Creates an instance of `any` containing an in-place initialized object of type `T`.
	 * @param args Arguments passed to constructor of `T`.
	 * @note Equivalent to `any(std::in_place_type<T>, std::forward<Args>(args)...)`. */
	template<typename T, typename... Args>
	[[nodiscard]] any make_any(Args &&...args)
	{
		return any{std::in_place_type<T>, std::forward<Args>(args)...};
	}
	/** @copybrief make_any
	 * @param args Arguments passed to constructor of `T`.
	 * @param il Initializer list passed to constructor of `T`.
	 * @note Equivalent to `any(std::in_place_type<T>, il, std::forward<Args>(args)...)`. */
	template<typename T, typename U, typename... Args>
	[[nodiscard]] any make_any(std::initializer_list<U> il, Args &&...args)
	{
		return any{std::in_place_type<T>, il, std::forward<Args>(args)...};
	}

	/** If the managed objects of `a` and `b` are of the same type that is equality comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_CORE_PUBLIC operator==(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is less-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_CORE_PUBLIC operator<(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is less-than-or-equal comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_CORE_PUBLIC operator<=(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is greater-than comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_CORE_PUBLIC operator>(const any &a, const any &b) noexcept;
	/** If the managed objects of `a` and `b` are of the same type that is greater-than-or-equal comparable,
	 * returns result of the comparison. Otherwise, returns `false`. */
	[[nodiscard]] bool SEK_CORE_PUBLIC operator>=(const any &a, const any &b) noexcept;

	namespace detail
	{
		any attr_data::get() const { return get_func(data); }

		void dtor_data::invoke(void *ptr) const { invoke_func(data, ptr); }
		any ctor_data::invoke(std::span<any> args) const { return invoke_func(data, args); }
		any func_overload::invoke(const void *ptr, std::span<any> args) const { return invoke_func(data, ptr, args); }

		any prop_data::get() const { return get_func(data); }
		void prop_data::set(const any &value) const { set_func(data, value); }

		template<typename T>
		constexpr any_vtable any_vtable::make_instance() noexcept
		{
			any_vtable result;

			if constexpr (std::copy_constructible<T>)
			{
				/* Copy-init & copy-assign operations are used only if there is no custom copy constructor. */
				result.copy_init = +[](const any &src, any &dst)
				{
					dst.m_storage.template init<T>(std::construct_at<T, const T &>, *src.m_storage.template get<T>());
					dst.m_storage.template init_flags<T>();
				};
				result.copy_assign = +[](const any &src, any &dst)
				{
					if (dst.is_ref()) [[unlikely]]
						dst.m_storage.template init<T>(std::construct_at<T, const T &>, *src.m_storage.template get<T>());
					else if constexpr (std::is_copy_assignable_v<T>)
						*dst.m_storage.template get<T>() = *src.m_storage.template get<T>();
					else
					{
						auto *ptr = dst.m_storage.template get<T>();
						std::destroy_at(ptr);
						std::construct_at(ptr, *src.m_storage.template get<T>());
					}
					dst.m_storage.template init_flags<T>();
				};
			}

			if constexpr (requires(const T &a, const T &b) { a == b; })
				result.cmp_eq = +[](const void *a, const void *b) -> bool
				{
					const auto &a_value = *static_cast<const T *>(a);
					const auto &b_value = *static_cast<const T *>(b);
					return a_value == b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a < b; })
				result.cmp_lt = +[](const void *a, const void *b) -> bool
				{
					const auto &a_value = *static_cast<const T *>(a);
					const auto &b_value = *static_cast<const T *>(b);
					return a_value < b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a <= b; })
				result.cmp_le = +[](const void *a, const void *b) -> bool
				{
					const auto &a_value = *static_cast<const T *>(a);
					const auto &b_value = *static_cast<const T *>(b);
					return a_value <= b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a > b; })
				result.cmp_gt = +[](const void *a, const void *b) -> bool
				{
					const auto &a_value = *static_cast<const T *>(a);
					const auto &b_value = *static_cast<const T *>(b);
					return a_value > b_value;
				};
			if constexpr (requires(const T &a, const T &b) { a >= b; })
				result.cmp_ge = +[](const void *a, const void *b) -> bool
				{
					const auto &a_value = *static_cast<const T *>(a);
					const auto &b_value = *static_cast<const T *>(b);
					return a_value >= b_value;
				};

			return result;
		}
	}	 // namespace detail

	template<typename... Args>
	any type_info::construct(std::in_place_t, Args &&...args) const
	{
		std::array<any, sizeof...(Args)> local_args = {forward_any(std::forward<Args>(args))...};
		return construct(std::span{local_args});
	}

	any attribute_info::value() const noexcept { return m_data->get(); }
	any constant_info::value() const noexcept { return m_data->get(); }
}	 // namespace sek
