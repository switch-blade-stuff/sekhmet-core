/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE
namespace sek::detail
{
	template<policy_t P>
	inline void vector_cross(vector_data<float, 3, P> &out,
							 const vector_data<float, 3, P> &l,
							 const vector_data<float, 3, P> &r) noexcept
	{
		const auto l_simd = x86_pack(l);
		const auto r_simd = x86_pack(r);
		const auto a = _mm_shuffle_ps(l_simd, l_simd, _MM_SHUFFLE(3, 0, 2, 1));
		const auto b = _mm_shuffle_ps(r_simd, r_simd, _MM_SHUFFLE(3, 1, 0, 2));
		const auto c = _mm_mul_ps(a, r_simd);
		x86_unpack(out, _mm_sub_ps(_mm_mul_ps(a, b), _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1))));
	}

#ifdef SEK_USE_SSE4_1
	template<policy_t P>
	inline float vector_dot(const vector_data<float, 3, P> &l, const vector_data<float, 3, P> &r) noexcept
	{
		const auto l_simd = x86_pack(l);
		const auto r_simd = x86_pack(r);
		return _mm_cvtss_f32(_mm_dp_ps(l_simd, r_simd, 0x71));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<float, 3, P> &out, const vector_data<float, 3, P> &l) noexcept
	{
		x86_vector_apply(out, l, [](auto v) { return _mm_div_ps(v, _mm_sqrt_ps(_mm_dp_ps(v, v, 0x7f))); })
	}
	template<policy_t P>
	constexpr void vector_reflect(vector_data<float, 3, P> &out,
								  const vector_data<float, 3, P> &i,
								  const vector_data<float, 3, P> &n) noexcept
	{
		const auto two = _mm_set1_ps(static_cast<U>(2.0));
		const auto i_simd = x86_pack(i);
		const auto n_simd = x86_pack(n);
		const auto dp = _mm_dp_ps(i_simd, n_simd, 0x7f);
		const auto dp2 = _mm_mul_ps(dp, two);
		const auto ndp = _mm_mul_ps(n_simd, dp2);
		x86_unpack(out, _mm_sub_ps(i_simd, ndp));
	}
	template<policy_t P>
	inline float vector_dot(const vector_data<float, 4, P> &l, const vector_data<float, 4, P> &r) noexcept
	{
		const auto l_simd = x86_pack(l);
		const auto r_simd = x86_pack(r);
		return _mm_cvtss_f32(_mm_dp_ps(l_simd, r_simd, 0xf1));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<float, 4, P> &out, const vector_data<float, 4, P> &l) noexcept
	{
		x86_vector_apply(out, l, [](auto v) { return _mm_div_ps(v, _mm_sqrt_ps(_mm_dp_ps(v, v, 0xff))); })
	}
	template<policy_t P>
	constexpr void vector_reflect(vector_data<float, 4, P> &out,
								  const vector_data<float, 4, P> &i,
								  const vector_data<float, 4, P> &n) noexcept
	{
		const auto two = _mm_set1_ps(static_cast<U>(2.0));
		const auto i_simd = x86_pack(i);
		const auto n_simd = x86_pack(n);
		const auto dp = _mm_dp_ps(i_simd, n_simd, 0xff);
		const auto dp2 = _mm_mul_ps(dp, two);
		const auto ndp = _mm_mul_ps(n_simd, dp2);
		x86_unpack(out, _mm_sub_ps(i_simd, ndp));
	}
#else
	template<std::size_t N, policy_t P>
	inline float vector_dot(const vector_data<float, N, P> &l, const vector_data<float, N, P> &r) noexcept
	{
		const auto l_simd = x86_pack(l);
		const auto r_simd = x86_pack(r);
		const auto a = _mm_mul_ps(r_simd, l_simd);
		const auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		const auto c = _mm_add_ps(a, b);
		return _mm_cvtss_f32(_mm_add_ss(c, _mm_movehl_ps(b, c)));
	}
	template<std::size_t N, policy_t P>
	inline void vector_norm(vector_data<float, N, P> &out, const vector_data<float, N, P> &l) noexcept
	{
		const auto l_simd = x86_pack(l);
		x86_unpack(out, _mm_div_ps(l_simd, _mm_sqrt_ps(_mm_set1_ps(vector_dot(l, l)))));
	}
	template<typename T, std::size_t N, policy_t P>
	constexpr void vector_reflect(vector_data<T, N, P> &out, const vector_data<T, N, P> &i, const vector_data<T, N, P> &n) noexcept
	{
		const auto i_simd = x86_pack(i);
		const auto n_simd = x86_pack(n);
		const auto a = _mm_mul_ps(i_simd, n_simd);
		const auto b = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 3, 0, 1));
		const auto c = _mm_add_ps(a, b);
		const auto dp = _mm_cvtss_f32(_mm_add_ss(c, _mm_movehl_ps(b, c)));
		const auto dp2 = _mm_set1_ps(dp * static_cast<U>(2.0));
		const auto ndp = _mm_mul_ps(n_simd, dp2);
		x86_unpack(out, _mm_sub_ps(i_simd, ndp));
	}
#endif

#if defined(SEK_USE_SSE2) && defined(SEK_USE_SSE4_1)
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
	{
		const auto l_simd = x86_pack(l);
		const auto r_simd = x86_pack(r);
		return _mm_cvtsd_f64(_mm_dp_pd(l_simd, r_simd, 0xf1));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l) noexcept
	{
		const auto l_simd = x86_pack(l);
		x86_unpack(out, _mm_div_pd(l_simd, _mm_sqrt_pd(_mm_dp_pd(l_simd, l_simd, 0xff))));
	}
	template<policy_t P>
	constexpr void vector_reflect(vector_data<float, 3, P> &out,
								  const vector_data<float, 3, P> &i,
								  const vector_data<float, 3, P> &n) noexcept
	{
		const auto two = _mm_set1_pd(static_cast<U>(2.0));
		const auto i_simd = x86_pack(i);
		const auto n_simd = x86_pack(n);
		const auto dp = _mm_dp_pd(i_simd, n_simd, 0xff);
		const auto dp2 = _mm_mul_pd(dp, two);
		const auto ndp = _mm_mul_pd(n_simd, dp2);
		x86_unpack(out, _mm_sub_pd(i_simd, ndp));
	}

#ifndef SEK_USE_AVX
	template<policy_t P>
	inline void vector_cross(vector_data<double, 3, P> &out,
							 const vector_data<double, 3, P> &l,
							 const vector_data<double, 3, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		/* 4 shuffles are needed here since the 3 doubles are split across 2 __m128d vectors. */
		const auto a = _mm_shuffle_pd(l.simd[0], l.simd[1], _MM_SHUFFLE2(0, 1));
		const auto b = _mm_shuffle_pd(r.simd[0], r.simd[1], _MM_SHUFFLE2(0, 1));

		out.simd[0] = _mm_sub_pd(_mm_mul_pd(a, _mm_shuffle_pd(r.simd[1], r.simd[0], _MM_SHUFFLE2(0, 0))),
								 _mm_mul_pd(b, _mm_shuffle_pd(l.simd[1], l.simd[0], _MM_SHUFFLE2(0, 0))));
		out.simd[1] = _mm_sub_pd(_mm_mul_pd(l.simd[0], b), _mm_mul_pd(r.simd[0], a));
	}
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 3, P> &l, const vector_data<double, 3, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.simd[0], r.simd[0], 0xf1),
			_mm_dp_pd(l.simd[1], r.simd[1], 0x11)));
		// clang-format on
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 3, P> &out, const vector_data<double, 3, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.simd[0], l.simd[0], 0xff),
			_mm_dp_pd(l.simd[1], l.simd[1], 0x1f)));
		// clang-format on
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
	template<policy_t P>
	constexpr void vector_reflect(vector_data<double, 3, P> &out,
								  const vector_data<double, 3, P> &i,
								  const vector_data<double, 3, P> &n) noexcept
	{
		const auto two = _mm_set1_pd(static_cast<U>(2.0));

		// clang-format off
		const auto dp2 = _mm_mul_pd(two, _mm_add_pd(
			_mm_dp_pd(i.simd[0], n.simd[0], 0xff),
			_mm_dp_pd(i.simd[1], n.simd[1], 0x1f))));
		// clang-format on

		const auto ndp0 = _mm_mul_ps(n.simd[0], dp2);
		const auto ndp1 = _mm_mul_ps(n.simd[1], dp2);
		out.simd[0] = _mm_sub_ps(i.simd[0], ndp0));
		out.simd[1] = _mm_sub_ps(i.simd[1], ndp1));
	}

	template<policy_t P>
	inline double vector_dot(const vector_data<double, 4, P> &l, const vector_data<double, 4, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		return _mm_cvtsd_f64(_mm_add_pd(
			_mm_dp_pd(l.simd[0], r.simd[0], 0xf1),
			_mm_dp_pd(l.simd[1], r.simd[1], 0xf1)));
		// clang-format on
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 4, P> &out, const vector_data<double, 4, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		// clang-format off
		const auto magn = _mm_sqrt_pd(_mm_add_pd(
			_mm_dp_pd(l.simd[0], l.simd[0], 0xff),
			_mm_dp_pd(l.simd[1], l.simd[1], 0xff)));
		// clang-format on
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
	template<policy_t P>
	constexpr void vector_reflect(vector_data<double, 4, P> &out,
								  const vector_data<double, 4, P> &i,
								  const vector_data<double, 4, P> &n) noexcept
	{
		const auto two = _mm_set1_pd(static_cast<U>(2.0));

		// clang-format off
		const auto dp2 = _mm_mul_pd(two, _mm_add_pd(
			_mm_dp_pd(i.simd[0], n.simd[0], 0xff),
			_mm_dp_pd(i.simd[1], n.simd[1], 0xff))));
		// clang-format on

		const auto ndp0 = _mm_mul_ps(n.simd[0], dp2);
		const auto ndp1 = _mm_mul_ps(n.simd[1], dp2);
		out.simd[0] = _mm_sub_ps(i.simd[0], ndp0));
		out.simd[1] = _mm_sub_ps(i.simd[0], ndp1));
	}
#endif
#else
	template<policy_t P>
	inline double vector_dot(const vector_data<double, 2, P> &l, const vector_data<double, 2, P> &r) noexcept
	{
		const auto l_simd = x86_pack(l);
		const auto r_simd = x86_pack(r);
		const auto a = _mm_mul_pd(r_simd, l_simd);
		const auto b = _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1));
		return _mm_cvtsd_f64(_mm_add_sd(a, b));
	}
	template<policy_t P>
	inline void vector_norm(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &l) noexcept
	{
		const auto l_simd = x86_pack(l);
		x86_unpack(out, _mm_div_pd(l_simd, _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l)))));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P>
	inline double vector_dot(const vector_data<double, N, P> &l, const vector_data<double, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const __m128d a[2] = {_mm_mul_pd(r.simd[0], l.simd[0]), _mm_mul_pd(r.simd[1], l.simd[1])};
		const __m128d b[2] = {_mm_shuffle_pd(a[0], a[0], _MM_SHUFFLE2(0, 1)), _mm_shuffle_pd(a[1], a[1], _MM_SHUFFLE2(0, 1))};
		return _mm_cvtsd_f64(_mm_add_sd(_mm_add_sd(a[0], b[0]), _mm_add_sd(a[1], b[1])));
	}
	template<std::size_t N, policy_t P>
	inline void vector_norm(vector_data<double, N, P> &out, const vector_data<double, N, P> &l) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto magn = _mm_sqrt_pd(_mm_set1_pd(vector_dot(l, l)));
		out.simd[0] = _mm_div_pd(l.simd[0], magn);
		out.simd[1] = _mm_div_pd(l.simd[1], magn);
	}
	template<std::size_t N, policy_t P>
	constexpr void vector_reflect(vector_data<double, N, P> &out,
								  const vector_data<double, N, P> &i,
								  const vector_data<double, N, P> &n) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		const auto a0 = _mm_mul_pd(i.simd[0], n.simd[0]);
		const auto a1 = _mm_mul_pd(i.simd[1], n.simd[1]);
		const auto b0 = _mm_shuffle_pd(a0, a0, _MM_SHUFFLE2(0, 1));
		const auto b1 = _mm_shuffle_pd(a1, a1, _MM_SHUFFLE2(0, 1));
		const auto dp = _mm_cvtsd_f64(_mm_add_sd(_mm_add_sd(a0, b0), _mm_add_sd(a1, b1)));
		const auto dp2 = _mm_set1_pd(dp * static_cast<U>(2.0));
		const auto ndp0 = _mm_mul_ps(n.simd[0], dp2);
		const auto ndp1 = _mm_mul_ps(n.simd[1], dp2);
		out.simd[0] = _mm_sub_ps(i.simd[0], ndp0));
		out.simd[1] = _mm_sub_ps(i.simd[1], ndp1));
	}
#endif
#endif
}	 // namespace sek::detail
#endif