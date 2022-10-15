/*
 * Created by switchblade on 2021-12-29
 */

#pragma once

#include "detail/vector/func/arithmetic.hpp"
#include "detail/vector/func/bitwise.hpp"
#include "detail/vector/func/category.hpp"
#include "detail/vector/func/exponential.hpp"
#include "detail/vector/func/geometric.hpp"
#include "detail/vector/func/relational.hpp"
#include "detail/vector/func/trigonometric.hpp"
#include "detail/vector/func/utility.hpp"
#include "detail/vector/type.hpp"

namespace sek
{
	template<typename T = float>
	using vec4 = basic_vec<T, 4>;
	template<typename T = float>
	using vec3 = basic_vec<T, 3>;
	template<typename T = float>
	using vec2 = basic_vec<T, 2>;
	template<typename T = float>
	using vec4_aligned = basic_vec<T, 4, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using vec3_aligned = basic_vec<T, 3, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using vec2_aligned = basic_vec<T, 2, policy_t::DEFAULT_SIMD>;

	typedef vec4<double> vec4d;
	typedef vec3<double> vec3d;
	typedef vec2<double> vec2d;
	typedef vec4<float> vec4f;
	typedef vec3<float> vec3f;
	typedef vec2<float> vec2f;
	typedef vec4<std::uint64_t> vec4ui64;
	typedef vec3<std::uint64_t> vec3ui64;
	typedef vec2<std::uint64_t> vec2ui64;
	typedef vec4<std::int64_t> vec4i64;
	typedef vec3<std::int64_t> vec3i64;
	typedef vec2<std::int64_t> vec2i64;
	typedef vec4<std::uint32_t> vec4ui;
	typedef vec3<std::uint32_t> vec3ui;
	typedef vec2<std::uint32_t> vec2ui;
	typedef vec4<std::int32_t> vec4i;
	typedef vec3<std::int32_t> vec3i;
	typedef vec2<std::int32_t> vec2i;
	typedef vec4_aligned<double> vec4d_aligned;
	typedef vec3_aligned<double> vec3d_aligned;
	typedef vec2_aligned<double> vec2d_aligned;
	typedef vec4_aligned<float> vec4f_aligned;
	typedef vec3_aligned<float> vec3f_aligned;
	typedef vec2_aligned<float> vec2f_aligned;
	typedef vec4_aligned<std::uint64_t> vec4ul_aligned;
	typedef vec3_aligned<std::uint64_t> vec3ul_aligned;
	typedef vec2_aligned<std::uint64_t> vec2ul_aligned;
	typedef vec4_aligned<std::int64_t> vec4l_aligned;
	typedef vec3_aligned<std::int64_t> vec3l_aligned;
	typedef vec2_aligned<std::int64_t> vec2l_aligned;
	typedef vec4_aligned<std::uint32_t> vec4ui_aligned;
	typedef vec3_aligned<std::uint32_t> vec3ui_aligned;
	typedef vec2_aligned<std::uint32_t> vec2ui_aligned;
	typedef vec4_aligned<std::int32_t> vec4i_aligned;
	typedef vec3_aligned<std::int32_t> vec3i_aligned;
	typedef vec2_aligned<std::int32_t> vec2i_aligned;

	template<typename T = float>
	using vec4_mask = vec_mask<basic_vec<T, 4>>;
	template<typename T = float>
	using vec3_mask = vec_mask<basic_vec<T, 3>>;
	template<typename T = float>
	using vec2_mask = vec_mask<basic_vec<T, 2>>;
	template<typename T = float>
	using vec4_mask_aligned = vec_mask<basic_vec<T, 4, policy_t::DEFAULT_SIMD>>;
	template<typename T = float>
	using vec3_mask_aligned = vec_mask<basic_vec<T, 3, policy_t::DEFAULT_SIMD>>;
	template<typename T = float>
	using vec2_mask_aligned = vec_mask<basic_vec<T, 2, policy_t::DEFAULT_SIMD>>;

	typedef vec4_mask<double> vec4d_mask;
	typedef vec3_mask<double> vec3d_mask;
	typedef vec2_mask<double> vec2d_mask;
	typedef vec4_mask<float> vec4f_mask;
	typedef vec3_mask<float> vec3f_mask;
	typedef vec2_mask<float> vec2f_mask;
	typedef vec4_mask<std::uint64_t> vec4ul_mask;
	typedef vec3_mask<std::uint64_t> vec3ul_mask;
	typedef vec2_mask<std::uint64_t> vec2ul_mask;
	typedef vec4_mask<std::int64_t> vec4l_mask;
	typedef vec3_mask<std::int64_t> vec3l_mask;
	typedef vec2_mask<std::int64_t> vec2l_mask;
	typedef vec4_mask<std::uint32_t> vec4ui_mask;
	typedef vec3_mask<std::uint32_t> vec3ui_mask;
	typedef vec2_mask<std::uint32_t> vec2ui_mask;
	typedef vec4_mask<std::int32_t> vec4i_mask;
	typedef vec3_mask<std::int32_t> vec3i_mask;
	typedef vec2_mask<std::int32_t> vec2i_mask;
	typedef vec4_mask_aligned<double> vec4d_mask_aligned;
	typedef vec3_mask_aligned<double> vec3d_mask_aligned;
	typedef vec2_mask_aligned<double> vec2d_mask_aligned;
	typedef vec4_mask_aligned<float> vec4f_mask_aligned;
	typedef vec3_mask_aligned<float> vec3f_mask_aligned;
	typedef vec2_mask_aligned<float> vec2f_mask_aligned;
	typedef vec4_mask_aligned<std::uint64_t> vec4ul_mask_aligned;
	typedef vec3_mask_aligned<std::uint64_t> vec3ul_mask_aligned;
	typedef vec2_mask_aligned<std::uint64_t> vec2ul_mask_aligned;
	typedef vec4_mask_aligned<std::int64_t> vec4l_mask_aligned;
	typedef vec3_mask_aligned<std::int64_t> vec3l_mask_aligned;
	typedef vec2_mask_aligned<std::int64_t> vec2l_mask_aligned;
	typedef vec4_mask_aligned<std::uint32_t> vec4ui_mask_aligned;
	typedef vec3_mask_aligned<std::uint32_t> vec3ui_mask_aligned;
	typedef vec2_mask_aligned<std::uint32_t> vec2ui_mask_aligned;
	typedef vec4_mask_aligned<std::int32_t> vec4i_mask_aligned;
	typedef vec3_mask_aligned<std::int32_t> vec3i_mask_aligned;
	typedef vec2_mask_aligned<std::int32_t> vec2i_mask_aligned;
}	 // namespace sek