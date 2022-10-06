/*
 * Created by switchblade on 2022-03-14
 */

#pragma once

#include "../../define.h"

namespace sek
{
	/** Fills a buffer with random bytes using OS-specific cryptographic source.
	 * In case no cryptographic source is available for this system this function will always fail.
	 * @param dest Destination buffer.
	 * @param len Length of the destination buffer.
	 * @return Amount of bytes filled, or -1 if a failure occurred. */
	SEK_API ssize_t sys_random(void *dest, std::size_t len) noexcept;
}	 // namespace sek