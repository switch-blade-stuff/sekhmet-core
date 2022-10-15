/*
 * Created by switchblade on 2021-12-16
 */

#pragma once

#include <bit>

#include "../../storage.hpp"

#ifdef SEK_ARCH_x86

#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>
#include <nmmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>

namespace sek::detail
{
	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle4_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_128_shuffle4_unwrap<J + 2>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle4_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_128_shuffle4_unwrap<0>(s);
	}

	template<std::size_t J, std::size_t I, std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle2_unwrap(std::index_sequence<I, Is...>) noexcept
	{
		constexpr auto bit = static_cast<std::uint8_t>(I) << J;
		if constexpr (sizeof...(Is) != 0)
			return bit | x86_128_shuffle2_unwrap<J + 1>(std::index_sequence<Is...>{});
		else
			return bit;
	}
	template<std::size_t... Is>
	constexpr std::uint8_t x86_128_shuffle2_mask(std::index_sequence<Is...> s) noexcept
	{
		return x86_128_shuffle2_unwrap<0>(s);
	}

	// clang-format off
#ifdef SEK_USE_SSE
	template<>
	struct mask_set<std::uint32_t>
	{
		template<typename U>
		constexpr void operator()(std::uint32_t &to, U &&from) const noexcept
		{
			to = from ? std::numeric_limits<std::uint32_t>::max() : 0;
		}
	};
	template<>
	struct mask_get<std::uint32_t>
	{
		constexpr bool operator()(auto &v) const noexcept { return v; }
	};

	template<std::size_t N, policy_t P> requires(N > 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	union mask_data<float, N, P>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

		constexpr mask_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i)
				values[i] = static_cast<bool>(data[i]) ? std::numeric_limits<std::uint32_t>::max() : 0;
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[N];
		__m128 simd;
	};
	template<std::size_t N, policy_t P> requires(N > 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	union vector_data<float, N, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr vector_data(const float (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i) values[i] = data[i];
		}

		[[nodiscard]] constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		[[nodiscard]] constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		[[nodiscard]] constexpr auto *data() noexcept { return values; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return values; }

		float values[N];
		__m128 simd;
	};

	template<std::size_t N, policy_t P>
	inline __m128 x86_pack(const vector_data<float, N, P> &v) noexcept requires(N <= 4)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			return v.simd;
		else if constexpr (N == 2)
		{
			const auto v64 = reinterpret_cast<const __m64 *>(v.data());
			return _mm_loadl_pi(_mm_setzero_ps(), v64);
		}
		else if constexpr (N == 3)
		{
			const auto v64 = reinterpret_cast<const __m64 *>(v.data());
			return _mm_loadh_pi(_mm_set_ss(v.data()[2]), v64);
		}
		else
			return _mm_loadu_ps(v.data());
	}
	template<std::size_t N, policy_t P>
	inline void x86_unpack(vector_data<float, N, P> &out, __m128 v) noexcept requires(N <= 4)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = v;
		else if constexpr (N == 2)
			_mm_storel_pi(reinterpret_cast<__m64 *>(out.data()), v);
		else if constexpr (N == 3)
		{
			_mm_storeh_pi(reinterpret_cast<__m64 *>(out.data()), v);
			_mm_store_ss(out.data() + 2, v);
		}
		else
			_mm_storeu_ps(v, out.data());
	}

	template<std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<float, N, P> &out, const vector_data<float, N, P> &v, F &&f)
	{
		x86_unpack(out, f(x86_pack(v)));
	}
	template<std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<float, N, P> &out, const vector_data<float, N, P> &a, const vector_data<float, N, P> &b, F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b)));
	}
	template<std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<float, N, P> &out,
									const vector_data<float, N, P> &a,
									const vector_data<float, N, P> &b,
									const vector_data<float, N, P> &c,
									F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b), x86_pack(c)));
	}

#ifdef SEK_USE_SSE2
	template<integral_of_size<4> T, std::size_t N, policy_t P> requires(N > 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	union mask_data<T, N, P>
	{
		using element_t = mask_element<std::uint32_t>;
		using const_element_t = mask_element<const std::uint32_t>;

		constexpr mask_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i)
				values[i] = static_cast<bool>(data[i]) ? std::numeric_limits<std::uint32_t>::max() : 0;
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint32_t values[N];
		__m128i simd;
	};
	template<integral_of_size<4> T, std::size_t N, policy_t P> requires(N > 2 && check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
	union vector_data<T, N, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr vector_data(const T (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i) values[i] = data[i];
		}

		[[nodiscard]] constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		[[nodiscard]] constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		[[nodiscard]] constexpr auto *data() noexcept { return values; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return values; }

		T values[N];
		__m128i simd;
	};

	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline __m128i x86_pack(const vector_data<T, N, P> &v) noexcept requires(N <= 4)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			return v.simd;
		else if constexpr (N == 2)
			return _mm_loadu_si64(v.data());
		else if constexpr (N == 3)
		{
			const auto vs = _mm_castsi128_ps(_mm_loadu_si32(v.data() + 2));
			const auto v64 = reinterpret_cast<const __m64 *>(v.data());
			return _mm_castps_si128(_mm_loadh_pi(vs, v64));
		}
		else
			return _mm_loadu_si128(reinterpret_cast<const __m128i *>(v.data()));
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P>
	inline void x86_unpack(vector_data<T, N, P> &out, __m128i v) noexcept requires(N <= 4)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = v;
		else if constexpr (N == 2)
			_mm_storeu_si64(v.data(), v);
		else if constexpr (N == 3)
		{
			const auto v64 = reinterpret_cast<__m64 *>(out.data());
			_mm_storeh_pi(v64, _mm_castsi128_ps(v));
			_mm_storeu_si32(out.data() + 2, v);
		}
		else
			_mm_storeu_si128(reinterpret_cast<__m128i *>(v.data()), v);
	}

	template<integral_of_size<4> T, std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<T, N, P> &out, const vector_data<T, N, P> &v, F &&f)
	{
		x86_unpack(out, f(x86_pack(v)));
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<T, N, P> &out, const vector_data<T, N, P> &a, const vector_data<T, N, P> &b, F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b)));
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<T, N, P> &out,
									const vector_data<T, N, P> &a,
									const vector_data<T, N, P> &b,
									const vector_data<T, N, P> &c,
									F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b), x86_pack(c)));
	}

	template<>
	struct mask_set<std::uint64_t>
	{
		template<typename U>
		constexpr void operator()(std::uint64_t &to, U &&from) const noexcept
		{
			to = from ? std::numeric_limits<std::uint64_t>::max() : 0;
		}
	};
	template<>
	struct mask_get<std::uint64_t>
	{
		constexpr bool operator()(auto &v) const noexcept { return v; }
	};

	template<policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union mask_data<double, 2, P>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(2, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[2];
		__m128d simd;
	};
	template<policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union vector_data<double, 2, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr explicit vector_data(const double (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(2, M); ++i) values[i] = data[i];
		}

		[[nodiscard]] constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		[[nodiscard]] constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		[[nodiscard]] constexpr auto *data() noexcept { return values; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return values; }

		double values[2];
		__m128d simd;
	};

	template<std::size_t N, policy_t P>
	inline __m128d x86_pack(const vector_data<double, 2, P> &v) noexcept
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			return v.simd;
		else
			return _mm_loadu_pd(v.data());
	}
	template<std::size_t N, policy_t P>
	inline void x86_unpack(vector_data<double, 2, P> &out, __m128d v) noexcept
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = v;
		else
			_mm_storeu_pd(out.data(), v);
	}

	template<policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &v, F &&f)
	{
		x86_unpack(out, f(x86_pack(v)));
	}
	template<policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<double, 2, P> &out, const vector_data<double, 2, P> &a, const vector_data<double, 2, P> &b, F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b)));
	}
	template<policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<double, 2, P> &out,
									const vector_data<double, 2, P> &a,
									const vector_data<double, 2, P> &b,
									const vector_data<double, 2, P> &c,
									F &&f)
	{
		if constexpr (!check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			x86_unpack(out, f(x86_pack(a), x86_pack(b), x86_pack(c)));
		else
			out.simd = f(a.simd, b.simd, c.simd);
	}

	template<integral_of_size<8> T, policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union mask_data<T, 2, P>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(2, M); ++i) operator[](i) = data[i];
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[2];
		__m128i simd;
	};
	template<integral_of_size<8> T, policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union vector_data<T, 2, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr explicit vector_data(const T (&data)[M]) noexcept
		{
			for (std::size_t i = 0; i < min<std::size_t>(2, M); ++i) values[i] = data[i];
		}

		[[nodiscard]] constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		[[nodiscard]] constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		[[nodiscard]] constexpr auto *data() noexcept { return values; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return values; }

		T values[2];
		__m128i simd;
	};

	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline __m128i x86_pack(const vector_data<T, 2, P> &v) noexcept
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			return v.simd;
		else
			return _mm_loadu_si128(reinterpret_cast<const __m128i *>(out.data()));
	}
	template<integral_of_size<8> T, std::size_t N, policy_t P>
	inline void x86_unpack(vector_data<T, 2, P> &out, __m128i v) noexcept
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
			out.simd = v;
		else
			_mm_storeu_si128(reinterpret_cast<__m128i *>(out.data()), v);
	}

	template<integral_of_size<8> T, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &v, F &&f)
	{
		x86_unpack(out, f(x86_pack(v)));
	}
	template<integral_of_size<8> T, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<T, 2, P> &out, const vector_data<T, 2, P> &a, const vector_data<T, 2, P> &b, F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b)));
	}
	template<integral_of_size<4> T, std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<T, 2, P> &out,
									const vector_data<T, 2, P> &a,
									const vector_data<T, 2, P> &b,
									const vector_data<T, 2, P> &c,
									F &&f)
	{
		x86_unpack(out, f(x86_pack(a), x86_pack(b), x86_pack(c)));
	}

#ifndef SEK_USE_AVX
	template<std::size_t N, policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union mask_data<double, N, P>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i)
				values[i] = static_cast<bool>(data[i]) ? std::numeric_limits<std::uint64_t>::max() : 0;
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[3];
		__m128d simd[2];
	};
	template<std::size_t N, policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union vector_data<double, N, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr vector_data(const double (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i) values[i] = data[i];
		}

		[[nodiscard]] constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		[[nodiscard]] constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		[[nodiscard]] constexpr auto *data() noexcept { return values; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return values; }

		double values[N];
		__m128d simd[2];
	};

	template<std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<double, N, P> &out, const vector_data<double, N, P> &v, F &&f)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = f(v.simd[0]);
			out.simd[1] = f(v.simd[1]);
		}
		else
		{
			auto v_simd = _mm_loadu_pd(v.data());
			_mm_storeu_pd(out.data(), f(v_simd));

			if constexpr (N > 3)
			{
				v_simd = _mm_set_sd(v.data()[2]);
				_mm_store_sd(out.data() + 2, f(v_simd));
			}
			else
			{
				v_simd = _mm_loadu_pd(v.data() + 2);
				_mm_storeu_pd(out.data() + 2, f(v_simd));
			}
		}
	}
	template<std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<double, N, P> &out, const vector_data<double, N, P> &a, const vector_data<double, N, P> &b, F &&f)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = f(a.simd[0], b.simd[0]);
			out.simd[1] = f(a.simd[1], b.simd[1]);
		}
		else
		{
			auto a_simd = _mm_loadu_pd(a.data());
			auto b_simd = _mm_loadu_pd(b.data());
			_mm_storeu_pd(out.data(), f(a_simd, b_simd));

			if constexpr (N > 3)
			{
				a_simd = _mm_set_sd(a.data()[2]);
				b_simd = _mm_set_sd(b.data()[2]);
				_mm_store_sd(out.data() + 2, f(a_simd, b_simd));
			}
			else
			{
				a_simd = _mm_loadu_pd(a.data() + 2);
				b_simd = _mm_loadu_pd(b.data() + 2);
				_mm_storeu_pd(out.data() + 2, f(a_simd, b_simd));
			}
		}
	}
	template<std::size_t N, policy_t P, typename F>
	constexpr void x86_vector_apply(vector_data<double, N, P> &out,
									const vector_data<double, N, P> &a,
									const vector_data<double, N, P> &b,
									const vector_data<double, N, P> &c,
									F &&f)
	{
		if constexpr (check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>)
		{
			out.simd[0] = f(a.simd[0], b.simd[0], c.simd[0]);
			out.simd[1] = f(a.simd[1], b.simd[1], c.simd[1]);
		}
		else
		{
			auto a_simd = _mm_loadu_pd(a.data());
			auto b_simd = _mm_loadu_pd(b.data());
			auto c_simd = _mm_loadu_pd(c.data());
			_mm_storeu_pd(out.data(), f(a_simd, b_simd, c_simd));

			if constexpr (N > 3)
			{
				a_simd = _mm_set_sd(a.data()[2]);
				b_simd = _mm_set_sd(b.data()[2]);
				c_simd = _mm_set_sd(c.data()[2]);
				_mm_store_sd(out.data() + 2, f(a_simd, b_simd, c_simd));
			}
			else
			{
				a_simd = _mm_loadu_pd(a.data() + 2);
				b_simd = _mm_loadu_pd(b.data() + 2);
				c_simd = _mm_loadu_pd(c.data() + 2);
				_mm_storeu_pd(out.data() + 2, f(a_simd, b_simd, c_simd));
			}
		}
	}

#ifndef SEK_USE_AVX2
	template<integral_of_size<8> T, std::size_t N, policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union mask_data<T, N, P>
	{
		using element_t = mask_element<std::uint64_t>;
		using const_element_t = mask_element<const std::uint64_t>;

		constexpr mask_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr mask_data(const bool (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i)
				values[i] = static_cast<bool>(data[i]) ? std::numeric_limits<std::uint64_t>::max() : 0;
		}

		constexpr element_t operator[](std::size_t i) noexcept { return {values[i]}; }
		constexpr const_element_t operator[](std::size_t i) const noexcept { return {values[i]}; }

		std::uint64_t values[3];
		__m128i simd[2];
	};
	template<integral_of_size<8> T, std::size_t N, policy_t P> requires check_policy_v<P, policy_t::STORAGE_MASK, policy_t::ALIGNED>
	union vector_data<T, N, P>
	{
		constexpr vector_data() noexcept : values{} {}
		template<std::size_t M>
		constexpr vector_data(const T (&data)[M]) noexcept : values{}
		{
			for (std::size_t i = 0; i < min(N, M); ++i) values[i] = data[i];
		}

		[[nodiscard]] constexpr auto &operator[](std::size_t i) noexcept { return values[i]; }
		[[nodiscard]] constexpr auto &operator[](std::size_t i) const noexcept { return values[i]; }

		[[nodiscard]] constexpr auto *data() noexcept { return values; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return values; }

		T values[N];
		__m128i simd[2];
	};
#endif
#endif
#endif
#endif
	// clang-format on
}	 // namespace sek::detail

#endif