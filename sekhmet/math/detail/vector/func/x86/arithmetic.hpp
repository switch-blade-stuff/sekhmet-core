/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::detail
{
	template<std::size_t N, policy_t P>
	inline void vector_add(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_add_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_sub(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_sub_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_mul(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_mul_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_div(vector_data<float, N, P> &out, const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_div_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_neg(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		x86_vector_apply(out, v, [](auto v) { return _mm_sub_ps(_mm_setzero_ps(), v); });
	}
	template<std::size_t N, policy_t P>
	inline void vector_abs(vector_data<float, N, P> &out, const vector_data<float, N, P> &v) noexcept
	{
		constexpr auto mask = std::bit_cast<float>(0x7fff'ffff);
		x86_vector_apply(out, v, [&](auto v) { return _mm_and_ps(_mm_set1_ps(mask), v); });
	}

	SEK_FORCE_INLINE __m128 x86_fmadd_ps(__m128 a, __m128 b, __m128 c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmadd_ps(a, b, c);
#else
		return _mm_add_ps(_mm_mul_ps(a, b), c);
#endif
	}
	SEK_FORCE_INLINE __m128 x86_fmsub_ps(__m128 a, __m128 b, __m128 c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmsub_ps(a, b, c);
#else
		return _mm_sub_ps(_mm_mul_ps(a, b), c);
#endif
	}
	template<std::size_t I, std::size_t N>
	SEK_FORCE_INLINE __m128 x86_polevl_ps_unroll(__m128 p, __m128 a, const float (&c)[N]) noexcept
	{
		if constexpr (I < N)
			return x86_polevl_ps_unroll<I + 1>(x86_fmadd_ps(p, a, _mm_set1_ps(c[I])), a, c);
		else
			return p;
	}
	template<std::size_t N>
	SEK_FORCE_INLINE __m128 x86_polevl_ps(__m128 a, const float (&c)[N]) noexcept
	{
		return x86_polevl_ps_unroll<1, N>(_mm_set1_ps(c[0]), a, c);
	}
	template<std::size_t N>
	SEK_FORCE_INLINE __m128 x86_polevl1_ps(__m128 a, const float (&c)[N]) noexcept
	{
		return x86_polevl_ps_unroll<0, N>(_mm_set1_ps(1.0), a, c);
	}

	template<std::size_t N, policy_t P>
	inline void vector_fmadd(vector_data<float, N, P> &out,
							 const vector_data<float, N, P> &a,
							 const vector_data<float, N, P> &b,
							 const vector_data<float, N, P> &c) noexcept
	{
		x86_vector_apply(out, a, b, c, x86_fmadd_ps);
	}
	template<std::size_t N, policy_t P>
	inline void vector_fmsub(vector_data<float, N, P> &out,
							 const vector_data<float, N, P> &a,
							 const vector_data<float, N, P> &b,
							 const vector_data<float, N, P> &c) noexcept
	{
		x86_vector_apply(out, a, b, c, x86_fmsub_ps);
	}

#ifdef SEK_USE_SSE2
	template<policy_t P>
	inline void vector_add(vector_data<double, 2, P> &out,
						   const vector_data<double, 2, P> &l,
						   const vector_data<double, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_add_pd);
	}
	template<policy_t P>
	inline void vector_sub(vector_data<double, 2, P> &out,
						   const vector_data<double, 2, P> &l,
						   const vector_data<double, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_sub_pd);
	}
	template<policy_t P>
	inline void vector_mul(vector_data<double, 2, P> &out,
						   const vector_data<double, 2, P> &l,
						   const vector_data<double, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_mul_pd);
	}
	template<policy_t P>
	inline void vector_div(vector_data<double, 2, P> &out,
						   const vector_data<double, 2, P> &l,
						   const vector_data<double, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_div_pd);
	}
	template<policy_t P>
	inline void vector_neg(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
	{
		x86_vector_apply(out, v, [](auto v) { return _mm_sub_pd(_mm_setzero_pd(), v); });
	}
	template<policy_t P>
	inline void vector_abs(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		x86_vector_apply(out, v, [&](auto v) { return _mm_and_pd(_mm_set1_pd(mask), v); });
	}

	SEK_FORCE_INLINE __m128d x86_fmadd_pd(__m128d a, __m128d b, __m128d c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmadd_pd(a, b, c);
#else
		return _mm_add_pd(_mm_mul_pd(a, b), c);
#endif
	}
	SEK_FORCE_INLINE __m128d x86_fmsub_pd(__m128d a, __m128d b, __m128d c) noexcept
	{
#ifdef SEK_USE_FMA
		return _mm_fmsub_pd(a, b, c);
#else
		return _mm_sub_pd(_mm_mul_pd(a, b), c);
#endif
	}
	template<std::size_t I, std::size_t N>
	SEK_FORCE_INLINE __m128d x86_polevl_pd_unroll(__m128d p, __m128d a, const double (&c)[N]) noexcept
	{
		if constexpr (I < N)
			return x86_polevl_pd_unroll<I + 1>(x86_fmadd_pd(p, a, _mm_set1_pd(c[I])), a, c);
		else
			return p;
	}
	template<std::size_t N>
	SEK_FORCE_INLINE __m128d x86_polevl_pd(__m128d a, const double (&c)[N]) noexcept
	{
		return x86_polevl_pd_unroll<1, N>(_mm_set1_pd(c[0]), a, c);
	}
	template<std::size_t N>
	SEK_FORCE_INLINE __m128d x86_polevl1_pd(__m128d a, const double (&c)[N]) noexcept
	{
		return x86_polevl_pd_unroll<0, N>(_mm_set1_pd(1.0), a, c);
	}

	template<policy_t P>
	inline void vector_fmadd(vector_data<double, 2, P> &out,
							 const vector_data<double, 2, P> &a,
							 const vector_data<double, 2, P> &b,
							 const vector_data<double, 2, P> &c) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, a, b, c, x86_fmadd_pd);
	}
	template<policy_t P>
	inline void vector_fmsub(vector_data<double, 2, P> &out,
							 const vector_data<double, 2, P> &a,
							 const vector_data<double, 2, P> &b,
							 const vector_data<double, 2, P> &c) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, a, b, c, x86_fmsub_pd);
	}

	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_add(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, l, r, _mm_add_epi32);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_sub(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_sub_epi32);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_neg(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
	{
		x86_vector_apply(out, v, [](auto v) { return _mm_sub_epi32(_mm_setzero_si128(), v); });
	}
#ifdef SEK_USE_SSSE3
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_abs(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
	{
		x86_vector_apply(out, v, _mm_abs_epi32);
	}
#endif

	template<integral_of_size<8> T, policy_t P>
	inline void vector_add(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_add_epi64);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_sub(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_sub_epi64);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_neg(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v) noexcept
	{
		x86_vector_apply(out, v, [](auto v) { return _mm_sub_epi64(_mm_setzero_si128(), v); });
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline void vector_add(vector_data<double, N, P> &out,
						   const vector_data<double, N, P> &l,
						   const vector_data<double, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_add_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_sub(vector_data<double, N, P> &out,
						   const vector_data<double, N, P> &l,
						   const vector_data<double, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_sub_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_mul(vector_data<double, N, P> &out,
						   const vector_data<double, N, P> &l,
						   const vector_data<double, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_mul_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_div(vector_data<double, N, P> &out,
						   const vector_data<double, N, P> &l,
						   const vector_data<double, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_div_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_neg(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, v, [z = _mm_setzero_pd()](auto v) { return _mm_sub_pd(z, v); });
	}
	template<std::size_t N, policy_t P>
	inline void vector_abs(vector_data<double, N, P> &out, const vector_data<double, N, P> &v) noexcept
	{
		constexpr auto mask = std::bit_cast<double>(0x7fff'ffff'ffff'ffff);
		x86_vector_apply(out, v, [m = _mm_set1_pd(mask)](auto v) { return _mm_and_pd(m, v); });
	}

	template<std::size_t N, policy_t P>
	inline void vector_fmadd(vector_data<double, N, P> &out,
							 const vector_data<double, N, P> &a,
							 const vector_data<double, N, P> &b,
							 const vector_data<double, N, P> &c) noexcept
	{
		x86_vector_apply(out, a, b, c, x86_fmadd_pd);
	}
	template<std::size_t N, policy_t P>
	inline void vector_fmsub(vector_data<double, N, P> &out,
							 const vector_data<double, N, P> &a,
							 const vector_data<double, N, P> &b,
							 const vector_data<double, N, P> &c) noexcept
	{
		x86_vector_apply(out, a, b, c, x86_fmsub_pd);
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_add(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_add_epi64);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_sub(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_sub_epi64);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_neg(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
	{
		x86_vector_apply(out, v, [z = _mm_setzero_si128()](auto v) { return _mm_sub_epi64(z, v); });
	}
#endif
#endif
#endif
}	 // namespace sek::detail
#endif