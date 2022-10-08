/*
 * Created by switchblade on 2021-12-29
 */

#pragma once

#include "detail/matrix.hpp"
#include "detail/matrix2x.hpp"
#include "detail/matrix3x.hpp"
#include "detail/matrix4x.hpp"

namespace sek
{
	template<typename T = float>
	using mat4x4 = basic_mat<T, 4, 4>;
	template<typename T = float>
	using mat4x3 = basic_mat<T, 4, 3>;
	template<typename T = float>
	using mat4x2 = basic_mat<T, 4, 2>;
	template<typename T = float>
	using mat3x4 = basic_mat<T, 3, 4>;
	template<typename T = float>
	using mat3x3 = basic_mat<T, 3, 3>;
	template<typename T = float>
	using mat3x2 = basic_mat<T, 3, 2>;
	template<typename T = float>
	using mat2x4 = basic_mat<T, 2, 4>;
	template<typename T = float>
	using mat2x3 = basic_mat<T, 2, 3>;
	template<typename T = float>
	using mat2x2 = basic_mat<T, 2, 2>;
	template<typename T = float>
	using mat4 = mat4x4<T>;
	template<typename T = float>
	using mat3 = mat3x3<T>;
	template<typename T = float>
	using mat2 = mat2x2<T>;

	template<typename T = float>
	using mat4x4_aligned = basic_mat<T, 4, 4, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat4x3_aligned = basic_mat<T, 4, 3, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat4x2_aligned = basic_mat<T, 4, 2, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat3x4_aligned = basic_mat<T, 3, 4, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat3x3_aligned = basic_mat<T, 3, 3, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat3x2_aligned = basic_mat<T, 3, 2, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat2x4_aligned = basic_mat<T, 2, 4, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat2x3_aligned = basic_mat<T, 2, 3, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat2x2_aligned = basic_mat<T, 2, 2, policy_t::DEFAULT_SIMD>;
	template<typename T = float>
	using mat4_aligned = mat4x4_aligned<T>;
	template<typename T = float>
	using mat3_aligned = mat3x3_aligned<T>;
	template<typename T = float>
	using mat2_aligned = mat2x2_aligned<T>;

	typedef mat4x4<double> mat4x4d;
	typedef mat4x3<double> mat4x3d;
	typedef mat4x2<double> mat4x2d;
	typedef mat3x4<double> mat3x4d;
	typedef mat3x3<double> mat3x3d;
	typedef mat3x2<double> mat3x2d;
	typedef mat2x4<double> mat2x4d;
	typedef mat2x3<double> mat2x3d;
	typedef mat2x2<double> mat2x2d;
	typedef mat4<double> mat4d;
	typedef mat3<double> mat3d;
	typedef mat2<double> mat2d;
	typedef mat4x4_aligned<double> mat4x4d_aligned;
	typedef mat4x3_aligned<double> mat4x3d_aligned;
	typedef mat4x2_aligned<double> mat4x2d_aligned;
	typedef mat3x4_aligned<double> mat3x4d_aligned;
	typedef mat3x3_aligned<double> mat3x3d_aligned;
	typedef mat3x2_aligned<double> mat3x2d_aligned;
	typedef mat2x4_aligned<double> mat2x4d_aligned;
	typedef mat2x3_aligned<double> mat2x3d_aligned;
	typedef mat2x2_aligned<double> mat2x2d_aligned;
	typedef mat4_aligned<double> mat4d_aligned;
	typedef mat3_aligned<double> mat3d_aligned;
	typedef mat2_aligned<double> mat2d_aligned;

	typedef mat4x4<float> mat4x4f;
	typedef mat4x3<float> mat4x3f;
	typedef mat4x2<float> mat4x2f;
	typedef mat3x4<float> mat3x4f;
	typedef mat3x3<float> mat3x3f;
	typedef mat3x2<float> mat3x2f;
	typedef mat2x4<float> mat2x4f;
	typedef mat2x3<float> mat2x3f;
	typedef mat2x2<float> mat2x2f;
	typedef mat4<float> mat4f;
	typedef mat3<float> mat3f;
	typedef mat2<float> mat2f;
	typedef mat4x4_aligned<float> mat4x4f_aligned;
	typedef mat4x3_aligned<float> mat4x3f_aligned;
	typedef mat4x2_aligned<float> mat4x2f_aligned;
	typedef mat3x4_aligned<float> mat3x4f_aligned;
	typedef mat3x3_aligned<float> mat3x3f_aligned;
	typedef mat3x2_aligned<float> mat3x2f_aligned;
	typedef mat2x4_aligned<float> mat2x4f_aligned;
	typedef mat2x3_aligned<float> mat2x3f_aligned;
	typedef mat2x2_aligned<float> mat2x2f_aligned;
	typedef mat4_aligned<float> mat4f_aligned;
	typedef mat3_aligned<float> mat3f_aligned;
	typedef mat2_aligned<float> mat2f_aligned;

	typedef mat4x4<std::uint64_t> mat4x4ul;
	typedef mat4x3<std::uint64_t> mat4x3ul;
	typedef mat4x2<std::uint64_t> mat4x2ul;
	typedef mat3x4<std::uint64_t> mat3x4ul;
	typedef mat3x3<std::uint64_t> mat3x3ul;
	typedef mat3x2<std::uint64_t> mat3x2ul;
	typedef mat2x4<std::uint64_t> mat2x4ul;
	typedef mat2x3<std::uint64_t> mat2x3ul;
	typedef mat2x2<std::uint64_t> mat2x2ul;
	typedef mat4x4<std::int64_t> mat4x4l;
	typedef mat4x3<std::int64_t> mat4x3l;
	typedef mat4x2<std::int64_t> mat4x2l;
	typedef mat3x4<std::int64_t> mat3x4l;
	typedef mat3x3<std::int64_t> mat3x3l;
	typedef mat3x2<std::int64_t> mat3x2l;
	typedef mat2x4<std::int64_t> mat2x4l;
	typedef mat2x3<std::int64_t> mat2x3l;
	typedef mat2x2<std::int64_t> mat2x2l;
	typedef mat4<std::uint64_t> mat4ul;
	typedef mat3<std::uint64_t> mat3ul;
	typedef mat2<std::uint64_t> mat2ul;
	typedef mat4<std::int64_t> mat4l;
	typedef mat3<std::int64_t> mat3l;
	typedef mat2<std::int64_t> mat2l;
	typedef mat4x4_aligned<std::uint64_t> mat4x4ul_aligned;
	typedef mat4x3_aligned<std::uint64_t> mat4x3ul_aligned;
	typedef mat4x2_aligned<std::uint64_t> mat4x2ul_aligned;
	typedef mat3x4_aligned<std::uint64_t> mat3x4ul_aligned;
	typedef mat3x3_aligned<std::uint64_t> mat3x3ul_aligned;
	typedef mat3x2_aligned<std::uint64_t> mat3x2ul_aligned;
	typedef mat2x4_aligned<std::uint64_t> mat2x4ul_aligned;
	typedef mat2x3_aligned<std::uint64_t> mat2x3ul_aligned;
	typedef mat2x2_aligned<std::uint64_t> mat2x2ul_aligned;
	typedef mat4x4_aligned<std::int64_t> mat4x4l_aligned;
	typedef mat4x3_aligned<std::int64_t> mat4x3l_aligned;
	typedef mat4x2_aligned<std::int64_t> mat4x2l_aligned;
	typedef mat3x4_aligned<std::int64_t> mat3x4l_aligned;
	typedef mat3x3_aligned<std::int64_t> mat3x3l_aligned;
	typedef mat3x2_aligned<std::int64_t> mat3x2l_aligned;
	typedef mat2x4_aligned<std::int64_t> mat2x4l_aligned;
	typedef mat2x3_aligned<std::int64_t> mat2x3l_aligned;
	typedef mat2x2_aligned<std::int64_t> mat2x2l_aligned;
	typedef mat4_aligned<std::uint64_t> mat4ul_aligned;
	typedef mat3_aligned<std::uint64_t> mat3ul_aligned;
	typedef mat2_aligned<std::uint64_t> mat2ul_aligned;
	typedef mat4_aligned<std::int64_t> mat4l_aligned;
	typedef mat3_aligned<std::int64_t> mat3l_aligned;
	typedef mat2_aligned<std::int64_t> mat2l_aligned;

	typedef mat4x4<std::uint32_t> mat4x4ui;
	typedef mat4x3<std::uint32_t> mat4x3ui;
	typedef mat4x2<std::uint32_t> mat4x2ui;
	typedef mat3x4<std::uint32_t> mat3x4ui;
	typedef mat3x3<std::uint32_t> mat3x3ui;
	typedef mat3x2<std::uint32_t> mat3x2ui;
	typedef mat2x4<std::uint32_t> mat2x4ui;
	typedef mat2x3<std::uint32_t> mat2x3ui;
	typedef mat2x2<std::uint32_t> mat2x2ui;
	typedef mat4x4<std::int32_t> mat4x4i;
	typedef mat4x3<std::int32_t> mat4x3i;
	typedef mat4x2<std::int32_t> mat4x2i;
	typedef mat3x4<std::int32_t> mat3x4i;
	typedef mat3x3<std::int32_t> mat3x3i;
	typedef mat3x2<std::int32_t> mat3x2i;
	typedef mat2x4<std::int32_t> mat2x4i;
	typedef mat2x3<std::int32_t> mat2x3i;
	typedef mat2x2<std::int32_t> mat2x2i;
	typedef mat4<std::uint32_t> mat4ui;
	typedef mat3<std::uint32_t> mat3ui;
	typedef mat2<std::uint32_t> mat2ui;
	typedef mat4<std::int32_t> mat4i;
	typedef mat3<std::int32_t> mat3i;
	typedef mat2<std::int32_t> mat2i;
	typedef mat4x4_aligned<std::uint32_t> mat4x4ui_aligned;
	typedef mat4x3_aligned<std::uint32_t> mat4x3ui_aligned;
	typedef mat4x2_aligned<std::uint32_t> mat4x2ui_aligned;
	typedef mat3x4_aligned<std::uint32_t> mat3x4ui_aligned;
	typedef mat3x3_aligned<std::uint32_t> mat3x3ui_aligned;
	typedef mat3x2_aligned<std::uint32_t> mat3x2ui_aligned;
	typedef mat2x4_aligned<std::uint32_t> mat2x4ui_aligned;
	typedef mat2x3_aligned<std::uint32_t> mat2x3ui_aligned;
	typedef mat2x2_aligned<std::uint32_t> mat2x2ui_aligned;
	typedef mat4x4_aligned<std::int32_t> mat4x4i_aligned;
	typedef mat4x3_aligned<std::int32_t> mat4x3i_aligned;
	typedef mat4x2_aligned<std::int32_t> mat4x2i_aligned;
	typedef mat3x4_aligned<std::int32_t> mat3x4i_aligned;
	typedef mat3x3_aligned<std::int32_t> mat3x3i_aligned;
	typedef mat3x2_aligned<std::int32_t> mat3x2i_aligned;
	typedef mat2x4_aligned<std::int32_t> mat2x4i_aligned;
	typedef mat2x3_aligned<std::int32_t> mat2x3i_aligned;
	typedef mat2x2_aligned<std::int32_t> mat2x2i_aligned;
	typedef mat4_aligned<std::uint32_t> mat4ui_aligned;
	typedef mat3_aligned<std::uint32_t> mat3ui_aligned;
	typedef mat2_aligned<std::uint32_t> mat2ui_aligned;
	typedef mat4_aligned<std::int32_t> mat4i_aligned;
	typedef mat3_aligned<std::int32_t> mat3i_aligned;
	typedef mat2_aligned<std::int32_t> mat2i_aligned;
}	 // namespace sek