/*
 * Created by switchblade on 10/08/22
 */

#include "../expected.hpp"

#if __cplusplus <= 202002L
sek::bad_expected_access<void>::~bad_expected_access() = default;
#endif