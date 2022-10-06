/*
 * Created by switchblade on 02/05/22
 */

#pragma once

#include "../vector.hpp"

#define SEK_DETAIL_MATRIX_COMMON(T, N, M, P)                                                                           \
private:                                                                                                               \
	template<typename U, std::size_t X, std::size_t Y, policy_t Q>                                                     \
	friend constexpr U *data(basic_mat<U, X, Y, Q> &m) noexcept;                                                       \
	template<typename U, std::size_t X, std::size_t Y, policy_t Q>                                                     \
	friend constexpr const U *data(const basic_mat<U, X, Y, Q> &) noexcept;                                            \
                                                                                                                       \
public:                                                                                                                \
	typedef T value_type;                                                                                              \
	typedef basic_vec<T, M, P> col_type;                                                                               \
	typedef basic_vec<T, N, P> row_type;                                                                               \
                                                                                                                       \
	/** Number of columns in the matrix. */                                                                            \
	constexpr static auto columns = N;                                                                                 \
	/** Number of rows in the matrix. */                                                                               \
	constexpr static auto rows = M;                                                                                    \
                                                                                                                       \
private:                                                                                                               \
	col_type m_data[N] = {}; /* Matrices stored as columns to optimize SIMD computation. */                            \
                                                                                                                       \
public:                                                                                                                \
	/** Initializes an identity matrix. */                                                                             \
	constexpr basic_mat() noexcept : basic_mat(1)                                                                      \
	{                                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	/** Initializes the main diagonal of the matrix to the provided value. */                                          \
	constexpr explicit basic_mat(T v) noexcept                                                                         \
	{                                                                                                                  \
		for (std::size_t i = 0; i < N && i < M; ++i) col(i)[i] = v;                                                    \
	}                                                                                                                  \
                                                                                                                       \
	constexpr explicit basic_mat(const col_type(&cols)[N]) noexcept                                                    \
	{                                                                                                                  \
		std::copy_n(cols, N, m_data);                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	/** Returns the corresponding column of the matrix. */                                                             \
	[[nodiscard]] constexpr col_type &operator[](std::size_t i) noexcept                                               \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr const col_type &operator[](std::size_t i) const noexcept                                   \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr col_type &col(std::size_t i) noexcept                                                      \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
	/** @copydoc operator[] */                                                                                         \
	[[nodiscard]] constexpr const col_type &col(std::size_t i) const noexcept                                          \
	{                                                                                                                  \
		return m_data[i];                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
	template<std::size_t... Is>                                                                                        \
	[[nodiscard]] constexpr row_type row(std::index_sequence<Is...>, std::size_t i) const noexcept                     \
	{                                                                                                                  \
		return row_type{m_data[Is][i]...};                                                                             \
	}                                                                                                                  \
                                                                                                                       \
public:                                                                                                                \
	/** Returns copy of the corresponding row of the matrix. */                                                        \
	[[nodiscard]] constexpr row_type row(std::size_t i) const noexcept                                                 \
	{                                                                                                                  \
		return row(std::make_index_sequence<columns>{}, i);                                                            \
	}                                                                                                                  \
                                                                                                                       \
	/** Returns pointer to the underlying data of the matrix.                                                          \
	 * @note Actual size of the data buffer might exceed `columns * rows` due to alignment requirements.               \
	 * Use `sizeof(basic_mat<T, N, M, P>)` to obtain the actual size of the matrix or use per-column `data()` function. */                                                                                                                    \
	[[nodiscard]] constexpr T *data() noexcept                                                                         \
		requires(requires(col_type & c) { c.data(); })                                                                 \
	{                                                                                                                  \
		return m_data[0].data();                                                                                       \
	}                                                                                                                  \
	/** @copydoc data */                                                                                               \
	[[nodiscard]] constexpr const T *data() const noexcept                                                             \
		requires(requires(const col_type &c) { c.data(); })                                                            \
	{                                                                                                                  \
		return m_data[0].data();                                                                                       \
	}                                                                                                                  \
                                                                                                                       \
	constexpr void swap(const basic_mat &other) noexcept                                                               \
	{                                                                                                                  \
		std::swap(m_data, other.m_data);                                                                               \
	}

namespace sek
{
	/** @brief Structure representing a mathematical matrix.
	 * Matrices are stored in column-major form.
	 * @tparam T Type of values stored in the matrix.
	 * @tparam N Amount of columns of the matrix.
	 * @tparam M Amount of rows of the matrix.
	 * @tparam Policy Policy used for storage & optimization.
	 * @note Generic matrix types are not guaranteed to be SIMD-optimized. */
	template<arithmetic T, std::size_t N, std::size_t M, policy_t Policy = policy_t::DEFAULT>
	class basic_mat;

	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool operator==(const basic_mat<T, N, M, Q> &a, const basic_mat<T, N, M, Q> &b) noexcept
	{
		auto mask = a[0] == b[0];
		for (std::size_t c = 1;; ++c)
		{
			if (!all(mask))
				return false;
			else if (c == N)
				return true;
			mask = a[c] == b[c];
		}
	}
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr bool operator!=(const basic_mat<T, N, M, Q> &a, const basic_mat<T, N, M, Q> &b) noexcept
	{
		return !operator==(a, b);
	}

	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr hash_t hash(const basic_mat<T, N, M, Q> &m) noexcept
	{
		hash_t result = 0;
		// clang-format off
		for (std::size_t i = 0; i < N; ++i)
			hash_combine(result, m[i]);
		// clang-format on
		return result;
	}
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr void swap(basic_mat<T, N, M, Q> &a, basic_mat<T, N, M, Q> &b) noexcept
	{
		a.swap(b);
	}

	/** Returns a matrix which is the result of addition of two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator+(const basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] + r[i];
		return result;
	}
	/** Adds a matrix to a matrix. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator+=(basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] += r[i];
		return l;
	}
	/** Returns a matrix which is the result of subtraction of two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator-(const basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] - r[i];
		return result;
	}
	/** Subtracts a matrix from a matrix. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator-=(basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] -= r[i];
		return l;
	}

	/** Returns a copy of a matrix multiplied by a scalar. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator*(const basic_mat<T, N, M, Q> &l, T r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] * r;
		return result;
	}
	/** @copydoc operator* */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator*(T l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		return r * l;
	}
	/** Multiplies matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator*=(basic_mat<T, N, M, Q> &l, T r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] *= r;
		return l;
	}
	/** Returns a copy of a matrix divided by a scalar. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator/(const basic_mat<T, N, M, Q> &l, T r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] / r;
		return result;
	}
	/** Returns a matrix produced by dividing a scalar by a matrix. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator/(T l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l / r[i];
		return result;
	}
	/** Divides matrix by a scalar. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator/=(basic_mat<T, N, M, Q> &l, T r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] /= r;
		return l;
	}

	/** Preforms a bitwise AND on two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator&=(basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] &= r[i];
		return l;
	}
	/** Returns a matrix which is the result of bitwise AND of two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator&(const basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] & r[i];
		return result;
	}
	/** Preforms a bitwise OR on two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator|=(basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] |= r[i];
		return l;
	}
	/** Returns a matrix which is the result of bitwise OR of two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator|(const basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] | r[i];
		return result;
	}
	/** Returns a matrix which is the result of bitwise XOR of two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator^(const basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = l[i] ^ r[i];
		return result;
	}
	/** Preforms a bitwise XOR on two matrices. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	constexpr basic_mat<T, N, M, Q> &operator^=(basic_mat<T, N, M, Q> &l, const basic_mat<T, N, M, Q> &r) noexcept
	{
		for (std::size_t i = 0; i < N; ++i) l[i] ^= r[i];
		return l;
	}
	/** Returns a bitwise inverted copy of a matrix. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator~(const basic_mat<T, N, M, Q> &m) noexcept
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = ~m[i];
		return result;
	}

	/** Returns a copy of the matrix. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator+(const basic_mat<T, N, M, Q> &m) noexcept
		requires std::is_signed_v<T>
	{
		return m;
	}
	/** Returns a negated copy of the matrix. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_mat<T, N, M, Q> operator-(const basic_mat<T, N, M, Q> &m) noexcept
		requires std::is_signed_v<T>
	{
		basic_mat<T, N, M, Q> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = -m[i];
		return result;
	}

	/** Returns a vector which is the result of multiplying matrix by a vector. */
	template<typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr basic_vec<T, M> operator*(const basic_mat<T, N, M, Q> &m, const basic_vec<T, N, Q> &v) noexcept
	{
		basic_vec<T, N> result = {};
		for (std::size_t i = 0; i < N; ++i) result += m[i] * v[i];
		return result;
	}
	/** Returns a vector which is the result of multiplying vector by a matrix. */
	template<typename T, std::size_t C0, std::size_t C1, policy_t Q>
	[[nodiscard]] constexpr basic_vec<T, C1, Q> operator*(const basic_vec<T, C0, Q> &v, const basic_mat<T, C1, C0, Q> &m) noexcept
	{
		basic_vec<T, C1> result = {};
		for (std::size_t i = 0; i < C1; ++i) result[i] = dot(v, m[i]);
		return result;
	}

	/** Gets the Ith column of the matrix. */
	template<std::size_t I, typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr typename basic_mat<T, N, M, Q>::col_type &get(basic_mat<T, N, M, Q> &m) noexcept
	{
		return m[I];
	}
	/** @copydoc get */
	template<std::size_t I, typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr const typename basic_mat<T, N, M, Q>::col_type &get(const basic_mat<T, N, M, Q> &m) noexcept
	{
		return m[I];
	}
	/** Gets the Jth element of the Ith column of the matrix. */
	template<std::size_t I, std::size_t J, typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr T &get(basic_mat<T, N, M, Q> &m) noexcept
	{
		return m[I][J];
	}
	template<std::size_t I, std::size_t J, typename T, std::size_t N, std::size_t M, policy_t Q>
	[[nodiscard]] constexpr const T &get(const basic_mat<T, N, M, Q> &m) noexcept
	{
		return m[I][J];
	}
}	 // namespace sek

template<typename T, std::size_t N, std::size_t M, sek::policy_t Q>
struct std::tuple_size<sek::basic_mat<T, N, M, Q>> : std::integral_constant<std::size_t, N>
{
};
template<std::size_t I, typename T, std::size_t N, std::size_t M, sek::policy_t Q>
struct std::tuple_element<I, sek::basic_mat<T, N, M, Q>>
{
	using type = typename sek::basic_mat<T, N, M, Q>::col_type;
};
