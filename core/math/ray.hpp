/*
 * Created by switchblade on 2022-10-01.
 */

#pragma once

#include "vector.hpp"

namespace sek
{
	/** @brief Structure representing a ray.
	 * @tparam T Value type of the ray's origin and direction vectors.
	 * @tparam N Dimension of the ray's origin and direction vectors.
	 * @tparam Policy Policy used for storage & optimization. */
	template<typename T, std::size_t N, policy_t Policy = policy_t::DEFAULT>
	struct basic_ray
	{
		typedef basic_vec<T, N> vec_type;

		vec_type origin;
		vec_type direction;
	};

	template<typename T = float>
	using ray4 = basic_ray<T, 4>;
	template<typename T = float>
	using ray3 = basic_ray<T, 3>;
	template<typename T = float>
	using ray2 = basic_ray<T, 2>;
	template<typename T = float>
	using ray4_aligned = basic_ray<T, 4, policy_t::DEFAULT_PACKED>;
	template<typename T = float>
	using ray3_aligned = basic_ray<T, 3, policy_t::DEFAULT_PACKED>;
	template<typename T = float>
	using ray2_aligned = basic_ray<T, 2, policy_t::DEFAULT_PACKED>;

	typedef ray4<double> ray4d;
	typedef ray3<double> ray3d;
	typedef ray2<double> ray2d;
	typedef ray4<float> ray4f;
	typedef ray3<float> ray3f;
	typedef ray2<float> ray2f;
	typedef ray4<std::uint64_t> ray4ul;
	typedef ray3<std::uint64_t> ray3ul;
	typedef ray2<std::uint64_t> ray2ul;
	typedef ray4<std::int64_t> ray4l;
	typedef ray3<std::int64_t> ray3l;
	typedef ray2<std::int64_t> ray2l;
	typedef ray4<std::uint32_t> ray4ui;
	typedef ray3<std::uint32_t> ray3ui;
	typedef ray2<std::uint32_t> ray2ui;
	typedef ray4<std::int32_t> ray4i;
	typedef ray3<std::int32_t> ray3i;
	typedef ray2<std::int32_t> ray2i;
	typedef ray4_aligned<double> ray4d_aligned;
	typedef ray3_aligned<double> ray3d_aligned;
	typedef ray2_aligned<double> ray2d_aligned;
	typedef ray4_aligned<float> ray4f_aligned;
	typedef ray3_aligned<float> ray3f_aligned;
	typedef ray2_aligned<float> ray2f_aligned;
	typedef ray4_aligned<std::uint64_t> ray4ul_aligned;
	typedef ray3_aligned<std::uint64_t> ray3ul_aligned;
	typedef ray2_aligned<std::uint64_t> ray2ul_aligned;
	typedef ray4_aligned<std::int64_t> ray4l_aligned;
	typedef ray3_aligned<std::int64_t> ray3l_aligned;
	typedef ray2_aligned<std::int64_t> ray2l_aligned;
	typedef ray4_aligned<std::uint32_t> ray4ui_aligned;
	typedef ray3_aligned<std::uint32_t> ray3ui_aligned;
	typedef ray2_aligned<std::uint32_t> ray2ui_aligned;
	typedef ray4_aligned<std::int32_t> ray4i_aligned;
	typedef ray3_aligned<std::int32_t> ray3i_aligned;
	typedef ray2_aligned<std::int32_t> ray2i_aligned;
}	 // namespace sek