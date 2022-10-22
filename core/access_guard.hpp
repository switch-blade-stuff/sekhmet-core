/*
 * Created by switchblade on 17/06/22
 */

#pragma once

#include <concepts>
#include <mutex>
#include <optional>
#include <utility>

#include <shared_mutex>

namespace sek
{
	namespace detail
	{
		// clang-format off
		template<typename T>
		concept basic_lockable = requires(T &value)
		{
			value.lock();
			value.unlock();
		};
		template<typename T>
		concept lockable = requires(T &value)
		{
			requires basic_lockable<T>;
			{ value.try_lock() } -> std::same_as<bool>;
		};
		template<typename T>
		concept shared_lockable = requires(T &value)
		{
			requires lockable<T>;
			value.lock_shared();
			value.unlock_shared();
			{ value.try_lock_shared() } -> std::same_as<bool>;
		};
		// clang-format on
	}	 // namespace detail

	template<typename P, detail::basic_lockable Mutex, bool>
	class access_guard;

	/** @brief Pointer-like accessor returned by `access_guard`. */
	template<typename P, typename L>
	class access_handle
	{
		template<typename, detail::basic_lockable, bool>
		friend class access_guard;

		using traits_t = std::pointer_traits<P>;

	public:
		typedef typename traits_t::element_type element_type;

	public:
		access_handle() = delete;
		access_handle(const access_handle &) = delete;
		access_handle &operator=(const access_handle &) = delete;

		// clang-format off
		constexpr access_handle(access_handle &&other) noexcept(std::is_nothrow_move_constructible_v<P> &&
																std::is_nothrow_move_constructible_v<L>)
			: m_ptr(std::move(other.m_ptr)), m_lock(std::move(other.m_lock)) {}
		constexpr access_handle &operator=(access_handle &&other) noexcept(std::is_nothrow_move_assignable_v<P> &&
																		   std::is_nothrow_move_assignable_v<L>)
		{
			m_ptr = std::move(other.m_ptr);
			m_lock = std::move(other.m_lock);
			return *this;
		}
		// clang-format on

		/** Initializes an access handle for pointer-like type ptr and lock `lock`. */
		constexpr access_handle(const P &ptr, L &&lock) : m_ptr(ptr), m_lock(std::move(lock)) {}
		/** @copydoc access_handle */
		constexpr access_handle(P &&ptr, L &&lock) : m_ptr(std::move(ptr)), m_lock(std::move(lock)) {}

		/** Returns the stored pointer as if via `std::to_address`. */
		[[nodiscard]] constexpr decltype(auto) get() const noexcept { return std::to_address(m_ptr); }
		/** @copydoc get */
		[[nodiscard]] constexpr decltype(auto) operator->() const noexcept { return get(); }
		/** Dereferences the guarded pointer. */
		[[nodiscard]] constexpr decltype(auto) operator*() const noexcept { return *m_ptr; }

		// clang-format off
		[[nodiscard]] constexpr auto operator<=>(const access_handle &other) const requires std::three_way_comparable<P>
		{
			return m_ptr <=> other.m_ptr;
		}
		[[nodiscard]] constexpr bool operator==(const access_handle &other) const requires std::equality_comparable<P>
		{
			return m_ptr == other.m_ptr;
		}
		// clang-format on

	private:
		P m_ptr;
		L m_lock;
	};

	/** @brief Smart pointer used to provide synchronized access to an instance of a type.
	 * @tparam P Pointer-like type guarded by the access guard.
	 * @tparam Mutex Type of mutex used to synchronize `T`. */
	template<typename P, detail::basic_lockable Mutex = std::mutex, bool = detail::shared_lockable<Mutex>>
	class access_guard
	{
		using traits_t = std::pointer_traits<P>;

	public:
		typedef Mutex mutex_type;
		typedef typename traits_t::element_type element_type;

		typedef std::unique_lock<Mutex> unique_lock;
		typedef access_handle<P, unique_lock> unique_handle;

	public:
		/** Initializes an empty access guard. */
		constexpr access_guard() noexcept = default;

		/** Initializes an access guard for a pointer of type `P` and mutex instance. */
		constexpr access_guard(const P &ptr, mutex_type *mtx) noexcept : m_ptr(ptr), m_mtx(mtx) {}
		/** @copydoc access_guard */
		constexpr access_guard(P &&ptr, mutex_type *mtx) noexcept : m_ptr(std::move(ptr)), m_mtx(mtx) {}

		/** Checks if the access guard is empty (does not point to any object). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr || m_mtx; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Acquires a unique lock and returns an accessor handle. */
		[[nodiscard]] constexpr unique_handle access() & { return {m_ptr, unique_lock{*m_mtx}}; }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle get() & { return access(); }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle operator->() & { return access(); }

		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle access() && { return {std::move(m_ptr), unique_lock{*m_mtx}}; }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle get() && { return access(); }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle operator->() && { return access(); }

		// clang-format off
		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<unique_handle> try_access() & requires detail::lockable<Mutex>
		{
			if (m_mtx->try_lock()) [[likely]]
				return {unique_handle{m_ptr, unique_lock{*m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
		/** @copydoc try_access */
		[[nodiscard]] constexpr std::optional<unique_handle> try_access() && requires detail::lockable<Mutex>
		{
			if (m_mtx->try_lock()) [[likely]]
				return {unique_handle{std::move(m_ptr), unique_lock{*m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
		// clang-format on

		/** Returns the underlying pointer. */
		[[nodiscard]] constexpr P &pointer() noexcept { return m_ptr; }
		/** @copydoc value */
		[[nodiscard]] constexpr const P &pointer() const noexcept { return m_ptr; }

		/** Returns pointer to the underlying mutex. */
		[[nodiscard]] constexpr mutex_type *mutex() noexcept { return m_mtx; }
		/** @copydoc mutex */
		[[nodiscard]] constexpr const mutex_type *mutex() const noexcept { return m_mtx; }

		// clang-format off
		[[nodiscard]] constexpr auto operator<=>(const access_guard &other) const requires std::three_way_comparable<P>
		{
			return m_ptr <=> other.m_ptr;
		}
		[[nodiscard]] constexpr bool operator==(const access_guard &other) const requires std::equality_comparable<P>
		{
			return m_ptr == other.m_ptr;
		}
		// clang-format on

	protected:
		P m_ptr = {};
		mutex_type *m_mtx = {};
	};
	/** @brief Overload of `access_guard` for shared mutex types. */
	template<typename P, detail::basic_lockable Mutex>
	class access_guard<P, Mutex, true> : public access_guard<P, Mutex, false>
	{
		using base_t = access_guard<P, Mutex, false>;
		using traits_t = std::pointer_traits<P>;

		using const_ptr = typename traits_t::template rebind<std::add_const_t<typename base_t::element_type>>;

		using base_t::m_ptr;

	public:
		typedef std::shared_lock<Mutex> shared_lock;
		typedef access_handle<const_ptr, shared_lock> shared_handle;

	public:
		using access_guard<P, Mutex, false>::operator=;
		using access_guard<P, Mutex, false>::operator->;
		using access_guard<P, Mutex, false>::access_guard;

		/** Acquires a shared lock and returns an accessor handle. */
		[[nodiscard]] constexpr shared_handle access_shared() const { return {*m_ptr, shared_lock{*base_t::m_mtx}}; }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle operator->() const { return access_shared(); }

		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<shared_handle> try_access_shared() const
		{
			if (base_t::m_mtx.try_lock_shared()) [[likely]]
				return {shared_handle{*m_ptr, shared_lock{*base_t::m_mtx, std::adopt_lock}}};
			else
				return {std::nullopt};
		}
	};

	template<typename P, typename M>
	access_guard(P &&, const M *) -> access_guard<P, M>;
	template<typename P, typename M>
	access_guard(const P &, const M *) -> access_guard<P, M>;

	/** @brief Type alias used to define an `access_guard` that uses an `std::recursive_mutex` as its mutex type. */
	template<typename P>
	using recursive_guard = access_guard<P, std::recursive_mutex>;
	/** @brief Type alias used to define an `access_guard` that uses an `std::shared_mutex` as its mutex type. */
	template<typename P>
	using shared_guard = access_guard<P, std::shared_mutex>;
}	 // namespace sek