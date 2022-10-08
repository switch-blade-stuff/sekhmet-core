/*
 * Created by switchblade on 25/06/22
 */

#pragma once

#include "common.hpp"

#ifdef SEK_USE_SSE2
namespace sek::detail
{
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_and(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_and_si128);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_xor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, l, r, _mm_xor_si128);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_or(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_or_si128);
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void vector_inv(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
	{
		x86_vector_apply(out, v, [m = _mm_set1_epi8(static_cast<std::int8_t>(0xff))](auto v) { return _mm_xor_si128(v, m); });
	}

	template<integral_of_size<8> T, policy_t P>
	inline void vector_and(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_and_si128);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_xor(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_xor_si128);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_or(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &l, const vector_data<T, 2, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_or_si128);
	}
	template<integral_of_size<8> T, policy_t P>
	inline void vector_inv(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v) noexcept
		requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	{
		x86_vector_apply(out, v, [m = _mm_set1_epi8(static_cast<std::int8_t>(0xff))](auto v) { return _mm_xor_si128(v, m); });
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_and(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_and_si128);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_xor(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_xor_si128);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_or(vector_data<T, N, P> &out, const vector_data<T, N, P> &l, const vector_data<T, N, P> &r) noexcept
	{
		x86_vector_apply(out, l, r, _mm_or_si128);
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void vector_inv(vector_data<T, N, P> &out, const vector_data<T, N, P> &v) noexcept
	{
		x86_vector_apply(out, v, [m = _mm_set1_epi8(static_cast<std::int8_t>(0xff))](auto v) { return _mm_xor_si128(v, m); });
	}
#endif
}	 // namespace sek::detail
#endif