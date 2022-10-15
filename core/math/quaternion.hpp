/*
 * Created by switchblade on 22/06/22
 */

#pragma once

#include "detail/quaternion/category.hpp"
#include "detail/quaternion/geometric.hpp"
#include "detail/quaternion/relational.hpp"
#include "detail/quaternion/type.hpp"

namespace sek
{
	using quatf = basic_quat<float>;
	using quatf_aligned = basic_quat<float, policy_t::DEFAULT_SIMD>;
	using quatd = basic_quat<double>;
	using quatd_aligned = basic_quat<double, policy_t::DEFAULT_SIMD>;
}	 // namespace sek