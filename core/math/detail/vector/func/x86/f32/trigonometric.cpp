/*
 * Created by switchblade on 26/06/22
 */

#include "trigonometric.hpp"

#include "../arithmetic.hpp"
#include "../exponential.hpp"

/* Implementations of trigonometric functions derived from netlib's cephes library (http://www.netlib.org/cephes/)
 * Inspired by http://gruntthepeon.free.fr/ssemath */

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	static const float sincof_f[3] = {-1.9515295891e-4f, 8.3321608736e-3f, -1.6666654611e-1f};
	static const float coscof_f[3] = {2.443315711809948e-5f, -1.388731625493765e-3f, 4.166664568298827e-2f};
	static const float dp_f[3] = {-0.78515625f, -2.4187564849853515625e-4f, -3.77489497744594108e-8f};
	static const float fopi_f = 4.0f / std::numbers::pi_v<float>; /* 4 / Pi */
	static const float pio2_f = std::numbers::pi_v<float> / 2.0f; /* Pi / 2 */
	static const float pio4_f = std::numbers::pi_v<float> / 4.0f; /* Pi / 4 */
	static const float pi_f = std::numbers::pi_v<float>;

	__m128 x86_sin_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(fopi_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				 /* c = (int32) b */
		/* c = (c + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		const auto flag = _mm_slli_epi32(_mm_and_si128(c, _mm_set1_epi32(4)), 29);		/* Swap sign flag */
		const auto sign = _mm_xor_ps(_mm_and_ps(v, sign_mask), _mm_castsi128_ps(flag)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi32(2));
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
		const auto select_mask = _mm_castsi128_ps(c);

		a = x86_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_mul_ps(_mm_mul_ps(x86_polevl_ps(a2, coscof_f), a2), a2); /* p1 = (coscof_f(a2) * a2) * a2 */
		p1 = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1);						   /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2  (Pi/4 <= a <= 0) */
		const auto p2 = x86_fmadd_ps(_mm_mul_ps(x86_polevl_ps(a2, sincof_f), a2), a, a); /* p2 = (sincof_f(a2) * a2 * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_ps(x86_blendv_ps(p1, p2, select_mask), sign);
	}
	__m128 x86_cos_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		auto a = _mm_and_ps(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(fopi_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				 /* c = (int32) b */
		/* j = (j + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		c = _mm_sub_epi32(c, _mm_set1_epi32(2));
		const auto sign = _mm_castsi128_ps(_mm_slli_epi32(_mm_andnot_si128(c, _mm_set1_epi32(4)), 29)); /* Extract sign bit */

		/* Calculate polynomial selection mask */
		c = _mm_and_si128(c, _mm_set1_epi32(2));
		c = _mm_cmpeq_epi32(c, _mm_setzero_si128());
		const auto select_mask = _mm_castsi128_ps(c);

		a = x86_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
		const auto a2 = _mm_mul_ps(a, a);

		/* P1 (0 <= a <= Pi/4) */
		auto p1 = _mm_mul_ps(_mm_mul_ps(x86_polevl_ps(a2, coscof_f), a2), a2); /* p1 = (coscof_f(a2) * a2) * a2 */
		p1 = x86_fmadd_ps(a2, _mm_set1_ps(-0.5f), p1);						   /* p1 = (a2 * -0.5) + p1 */
		p1 = _mm_add_ps(p1, _mm_set1_ps(1.0f));

		/* P2 (Pi/4 <= a <= 0) */
		const auto p2 = x86_fmadd_ps(_mm_mul_ps(x86_polevl_ps(a2, sincof_f), a2), a, a); /* p2 = (sincof_f(a2) * a2 * a) + a */

		/* Choose between P1 and P2 */
		return _mm_xor_ps(x86_blendv_ps(p1, p2, select_mask), sign);
	}

	static const float tancof_f[6] = {
		9.38540185543e-3f,
		3.11992232697e-3f,
		2.44301354525e-2f,
		5.34112807005e-2f,
		1.33387994085e-1f,
		3.33331568548e-1f,
	};

	inline static __m128 x86_tancot_ps(__m128 v, __m128i cot_mask) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto sign = _mm_and_ps(v, sign_mask);

		auto a = _mm_and_ps(v, abs_mask);			 /* a = |v| */
		auto b = _mm_mul_ps(a, _mm_set1_ps(fopi_f)); /* b = a * (4 / PI) */
		auto c = _mm_cvttps_epi32(b);				 /* c = (int32) b */

		/* c = (c + 1) & (~1) */
		c = _mm_add_epi32(c, _mm_set1_epi32(1));
		c = _mm_and_si128(c, _mm_set1_epi32(~1));
		b = _mm_cvtepi32_ps(c);

		/* Calculate polynomial selection mask */
		const auto select_mask = _mm_cmpngt_ps(a, _mm_set1_ps(0.0001f));

		a = x86_fmadd_ps(_mm_set1_ps(dp_f[0]), b, a); /* a = (dp_f[0] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[1]), b, a); /* a = (dp_f[1] * b) + a */
		a = x86_fmadd_ps(_mm_set1_ps(dp_f[2]), b, a); /* a = (dp_f[2] * b) + a */
		const auto a2 = _mm_mul_ps(a, a);

		/* p = a > 0.0001 ? poly(a, coscof_d) : a */
		auto p = _mm_mul_ps(x86_polevl_ps(a2, tancof_f), a2);	  /* p = tancof_f(a2) * a2 */
		p = x86_blendv_ps(x86_fmadd_ps(p, a, a), a, select_mask); /* p = select_mask ? a : (p * a) + a */

		const auto bit2 = _mm_cmpeq_epi32(_mm_and_si128(c, _mm_set1_epi32(2)), _mm_set1_epi32(2));
		const auto select1 = _mm_castsi128_ps(_mm_and_si128(bit2, cot_mask));	 /* select1 = (c & 2) && cot_mask */
		const auto select2 = _mm_castsi128_ps(_mm_andnot_si128(cot_mask, bit2)); /* select2 = (c & 2) && !cot_mask */
		const auto select3 = _mm_castsi128_ps(_mm_andnot_si128(bit2, cot_mask)); /* select3 = !(c & 2) && cot_mask */
		const auto p1 = _mm_xor_ps(p, sign_mask);								 /* p1 = -p */
		const auto p2 = _mm_div_ps(_mm_set1_ps(-1.0f), p);						 /* p2 = -1.0 / p */
		const auto p3 = _mm_div_ps(_mm_set1_ps(1.0f), p);						 /* p3 = 1.0 / p */

		p = x86_blendv_ps(p, p3, select3); /* p = select3 ? p3 : p */
		p = x86_blendv_ps(p, p2, select2); /* p = select2 ? p2 : result */
		p = x86_blendv_ps(p, p1, select1); /* p = select1 ? p1 : result */
		return _mm_xor_ps(p, sign);
	}
	__m128 x86_tan_ps(__m128 v) noexcept { return x86_tancot_ps(v, _mm_setzero_si128()); }
	__m128 x86_cot_ps(__m128 v) noexcept { return x86_tancot_ps(v, _mm_set1_epi32(-1)); }

	static const float sinhcof_f[3] = {2.03721912945e-4f, 8.33028376239e-3f, 1.66667160211e-1f};

	__m128 x86_sinh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P1 (a > 1.0) */
		auto p1 = x86_exp_ps(a);
		const auto tmp = _mm_div_ps(_mm_set1_ps(-0.5f), p1); /* tmp = (-0.5 / p1) */
		p1 = x86_fmadd_ps(_mm_set1_ps(0.5f), p1, tmp);		 /* p1 = (0.5 * p1) + tmp */
		p1 = _mm_xor_ps(p1, _mm_and_ps(v, sign_mask));		 /* p1 = v < 0 ? -p1 : p1 */

		/* P2 (a <= 1.0) */
		const auto v2 = _mm_mul_ps(v, v);
		auto p2 = x86_polevl_ps(v2, sinhcof_f);		 /* p2 = sinhcof_f(v2) * v2 */
		p2 = x86_fmadd_ps(_mm_mul_ps(p2, v2), v, v); /* p2 = ((p2 * v2) * v) + v */

		/* return (a > 1.0) ? p1 : p2 */
		return x86_blendv_ps(p1, p2, _mm_cmpngt_ps(a, _mm_set1_ps(1.0f)));
	}
	__m128 x86_cosh_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		auto a = x86_exp_ps(_mm_and_ps(v, abs_mask));		 /* a = exp(|v|) */
		a = _mm_add_ps(_mm_div_ps(_mm_set1_ps(1.0f), a), a); /* a = 1.0 / a + a */
		return _mm_mul_ps(a, _mm_set1_ps(0.5f));			 /* return a * 0.5 */
	}

	static const float tanhcof_f[5] = {
		-5.70498872745e-3f,
		2.06390887954e-2f,
		-5.37397155531e-2f,
		1.33314422036e-1f,
		-3.33332819422e-1f,
	};

	__m128 x86_tanh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P1 (a >= 0.625) */
		auto p1 = x86_exp_ps(_mm_add_ps(a, a));
		p1 = _mm_add_ps(_mm_set1_ps(1.0f), p1);		   /* p1 = 1.0 + p1 */
		p1 = _mm_div_ps(_mm_set1_ps(2.0f), p1);		   /* p1 = 2.0 / p1 */
		p1 = _mm_sub_ps(_mm_set1_ps(1.0f), p1);		   /* p1 = 1.0 - p1 */
		p1 = _mm_xor_ps(_mm_and_ps(v, sign_mask), p1); /* p1 = v < 0 ? -p1 : p1 */

		/* P1 (a < 0.625) */
		const auto a2 = _mm_mul_ps(a, a);
		auto p2 = x86_polevl_ps(a2, tanhcof_f);		 /* p2 = tanhcof_f(a2) */
		p2 = x86_fmadd_ps(_mm_mul_ps(p2, a2), v, v); /* p2 = ((p2 * a2) * v) + v */

		/* return (a >= 0.625) ? p1 : p2 */
		return x86_blendv_ps(p1, p2, _mm_cmplt_ps(a, _mm_set1_ps(0.625f)));
	}

	static const float asincof_f[5] = {
		4.2163199048e-2f,
		2.4181311049e-2f,
		4.5470025998e-2f,
		7.4953002686e-2f,
		1.6666752422e-1f,
	};

	__m128 x86_asin_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* P (a >= 0.0001) */
		const auto half = _mm_set1_ps(0.5f);
		const auto half_mask = _mm_cmpngt_ps(a, half);			   /* half_mask = a > 0.5 */
		const auto c1 = x86_fmadd_ps(a, _mm_set1_ps(-0.5f), half); /* c1 = (a * -0.5) + 0.5 */
		const auto b1 = _mm_sqrt_ps(c1);						   /* b1 = sqrt(c1) */
		const auto c2 = _mm_mul_ps(v, v);						   /* c2 = v * v */
		const auto b2 = a;										   /* b2 = a */

		const auto b = x86_blendv_ps(b1, b2, half_mask); /* b = (a > 0.5) ? b1 : b2 */
		const auto c = x86_blendv_ps(c1, c2, half_mask); /* c = (a > 0.5) ? c1 : c2 */

		auto p = x86_polevl_ps(c, asincof_f);	  /* p = asincof_f(c) */
		p = x86_fmadd_ps(_mm_mul_ps(p, c), b, b); /* p = ((p * c) * b) + b */

		/* p = half_mask ? (Pi / 2) - (p + p) : p */
		p = x86_blendv_ps(_mm_sub_ps(_mm_set1_ps(pio2_f), _mm_add_ps(p, p)), p, half_mask);

		/* result = (a < 0.0001) ? a : p */
		const auto result = x86_blendv_ps(a, p, _mm_cmpnlt_ps(a, _mm_set1_ps(0.0001f)));
		return _mm_xor_ps(result, _mm_and_ps(v, sign_mask)); /* return (v < 0) ? -result : result */
	}
	__m128 x86_acos_ps(__m128 v) noexcept
	{
		const auto half_minus = _mm_set1_ps(-0.5f);
		const auto half = _mm_set1_ps(0.5f);

		/* v < -0.5 */
		auto a = x86_asin_ps(_mm_sqrt_ps(x86_fmadd_ps(v, half, half))); /* a = asinf(sqrtf((v * 0.5) + 0.5)) */
		a = x86_fmadd_ps(a, _mm_set1_ps(-2.0f), _mm_set1_ps(pi_f));		/* a = (a * -2.0) + pi */

		/* v > 0.5 */
		auto b = x86_fmadd_ps(v, half_minus, half);						/* b = (v * -0.5) + 0.5 */
		b = _mm_mul_ps(_mm_set1_ps(2.0f), x86_asin_ps(_mm_sqrt_ps(b))); /* b = 2.0 * asinf(sqrtf(b)) */

		/* |v| <= 0.5 */
		const auto c = _mm_sub_ps(_mm_set1_ps(pio2_f), x86_asin_ps(v));

		/* Calculate masks & select between a, b & c */
		const auto a_mask = _mm_cmpnlt_ps(v, half_minus);
		const auto b_mask = _mm_cmpngt_ps(v, half);
		return x86_blendv_ps(a, x86_blendv_ps(b, c, b_mask), a_mask); /* return (v < -0.5) ? a : ((v > 0.5) ? b : c) */
	}

	static const float atancof_f[4] = {8.05374449538e-2f, -1.38776856032e-1f, 1.99777106478e-1f, -3.33329491539e-1f};
	static const float tan3pi8_f = 2.414213562373095f; /* tan((3 * Pi) / 8) */
	static const float tanpi8_f = 0.4142135623730950f; /* tan(Pi / 8) */

	/* TODO: Find a better algorithm for arc tangent. Precision of this is dubious at best.
	 * Potential candidates:
	 *  1. `newlib (fdlibm)` atanf - High precision, but a lot of branches. Probably unsuitable for SIMD. */
	__m128 x86_atan_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* Range reduction */
		const auto select1 = _mm_cmpngt_ps(a, _mm_set1_ps(tan3pi8_f)); /* a > tan3pi8_f */
		const auto select2 = _mm_cmpngt_ps(a, _mm_set1_ps(tanpi8_f));  /* a > tanpi8_f */
		/* if (a > tan3pi8_f) */
		const auto a1 = _mm_div_ps(_mm_set1_ps(-1.0f), a); /* a = -1.0 / a */
		const auto b1 = _mm_set1_ps(pio2_f);
		/* else if (a > tanpi8_f) */
		const auto one = _mm_set1_ps(1.0f);
		const auto a2 = _mm_div_ps(_mm_sub_ps(a, one), _mm_add_ps(a, one)); /* a = (a - 1.0) / (a + 1.0) */
		const auto b2 = _mm_set1_ps(pio4_f);

		auto b = _mm_setzero_ps();
		a = x86_blendv_ps(a1, a, select1); /* b = (a > tan3pi8_f) ? a1 : a */
		b = x86_blendv_ps(b1, b, select1); /* b = (a > tan3pi8_f) ? b1 : b */
		a = x86_blendv_ps(a2, a, select2); /* b = (a > tanpi8_f) ? a2 : a */
		b = x86_blendv_ps(b2, b, select2); /* b = (a > tanpi8_f) ? b2 : b */

		const auto c = _mm_mul_ps(a, a);
		auto p = x86_polevl_ps(c, atancof_f);	  /* p = atancof_f(c) */
		p = x86_fmadd_ps(_mm_mul_ps(p, c), a, a); /* p = ((p * c) * a) + a */

		return _mm_xor_ps(_mm_add_ps(b, p), _mm_and_ps(v, sign_mask)); /* return v < 0 ? -(b + p) : (b + p) */
	}

	static const float asinhcof_f[4] = {2.0122003309e-2f, -4.2699340972e-2f, 7.4847586088e-2f, -1.6666288134e-1f};
	static const float acoshcof_f[5] = {1.7596881071e-3f, -7.5272886713e-3f, 2.6454905019e-2f, -1.1784741703e-1f, 1.4142135263E0f};
	static const float loge2_f = 0.693147180559945309f;

	__m128 x86_asinh_ps(__m128 v) noexcept
	{
		const auto sign_mask = _mm_set1_ps(std::bit_cast<float>(0x8000'0000));
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));
		const auto a = _mm_and_ps(v, abs_mask); /* a = |v| */

		/* a > 1500.0 */
		const auto b1 = _mm_add_ps(x86_log_ps(a), _mm_set1_ps(loge2_f)); /* b1 = log(a) + loge2_f */

		/* a <= 1500.0 && a < 0.5 */
		const auto a2 = _mm_mul_ps(a, a);
		auto b2 = x86_polevl_ps(a2, asinhcof_f);	 /* b2 = asinhcof_f(a2) */
		b2 = x86_fmadd_ps(_mm_mul_ps(b2, a2), a, a); /* b2 = ((b2 * a2) * a) + a */

		/* a <= 1500.0 && a >= 0.5 */
		const auto tmp = _mm_sqrt_ps(_mm_add_ps(a2, _mm_set1_ps(1.0f))); /* tmp = sqrt(a2 + 1.0) */
		const auto b3 = x86_log_ps(_mm_add_ps(a, tmp));					 /* b3 = log(a + tmp) */

		/* b = (a > 1500.0) ? b1 : ((a < 0.5) ? b2 : b3) */
		const auto select1 = _mm_cmpngt_ps(a, _mm_set1_ps(1500.0f));			   /* a > 1500.0 */
		const auto select2 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.5f));				   /* a < 0.5 */
		const auto b = x86_blendv_ps(b1, x86_blendv_ps(b2, b3, select2), select1); /* b = (select2) ? b1 : (select1) ? b2 : b3 */
		return _mm_xor_ps(b, _mm_and_ps(v, sign_mask));							   /* return v < 0 ? -b : b */
	}
	__m128 x86_acosh_ps(__m128 v) noexcept
	{
		const auto a = _mm_sub_ps(v, _mm_set1_ps(1.0f)); /* a = v - 1.0 */

		/* v > 1500.0 */
		const auto b1 = _mm_add_ps(x86_log_ps(v), _mm_set1_ps(loge2_f)); /* b1 = log(v) + loge2_f */

		/* v <= 1500.0 && a < 0.5 */
		auto b2 = x86_polevl_ps(a, acoshcof_f); /* b2 = acoshcof_f(a) */
		b2 = _mm_mul_ps(b2, _mm_sqrt_ps(a));	/* b2 = b2 * sqrtf(a) */

		/* a <= 1500.0 && a >= 0.5 */
		const auto b3 = x86_log_ps(_mm_add_ps(v, _mm_sqrt_ps(x86_fmadd_ps(v, a, a)))); /* b3 = logf(v + sqrt((v * a) + a)) */

		const auto select1 = _mm_cmpngt_ps(v, _mm_set1_ps(1500.0f));	   /* v > 1500.0 */
		const auto select2 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.5f));		   /* a < 0.5 */
		return x86_blendv_ps(b1, x86_blendv_ps(b2, b3, select2), select1); /* return (select1) ? b1 : (select2) ? b2 : b3 */
	}

	static const float atanhcof_f[5] = {
		1.81740078349e-1f,
		8.24370301058e-2f,
		1.46691431730e-1f,
		1.99782164500e-1f,
		3.33337300303e-1f,
	};

	__m128 x86_atanh_ps(__m128 v) noexcept
	{
		const auto abs_mask = _mm_set1_ps(std::bit_cast<float>(0x7fff'ffff));

		/* a >= 0.0001 && a >= 0.5 */
		const auto one = _mm_set1_ps(1.0f);
		const auto tmp = _mm_div_ps(_mm_add_ps(one, v), _mm_sub_ps(one, v)); /* tmp = (1.0 + v) / (1.0 - v) */
		const auto a2 = _mm_mul_ps(_mm_set1_ps(0.5f), x86_log_ps(tmp));		 /* a2 = 0.5 * log(tmp); */

		/* a >= 0.0001 && a < 0.5 */
		const auto v2 = _mm_mul_ps(v, v);
		auto a1 = x86_polevl_ps(v2, atanhcof_f);	 /* a1 = atanhcof_f(v2) */
		a1 = x86_fmadd_ps(_mm_mul_ps(a1, v2), v, v); /* a1 = ((a1 * v2) * v) + v */

		const auto a = _mm_and_ps(v, abs_mask);							  /* a = |v| */
		const auto select1 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.5f));		  /* a < 0.5 */
		const auto select2 = _mm_cmpnlt_ps(a, _mm_set1_ps(0.0001f));	  /* a < 0.0001 */
		return x86_blendv_ps(v, x86_blendv_ps(a1, a2, select1), select2); /* a = (select2) ? v : (select1) ? a1 : a2 */
	}
}	 // namespace sek::detail
#endif
