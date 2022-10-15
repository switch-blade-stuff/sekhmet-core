/*
 * Created by switchblade on 10/14/22.
 */

#pragma once

#include <fcntl.h>

#include "../../../define.h"
#include <system_error>

#if (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS >= 64) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L)
#define FTRUNCATE ::ftruncate
#define LSEEK ::lseek
#define MMAP ::mmap
#define OFF_T ::off_t
#elif defined(_LARGEFILE64_SOURCE)
#define FTRUNCATE ::ftruncate64
#define LSEEK ::lseek64
#define MMAP ::mmap64
#define OFF_T ::off64_t
#endif

#if (defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 16))) ||                            \
	(defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L))
#define USE_FSYNC
#endif

namespace sek::detail
{
	constexpr auto access = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

	inline static std::error_code current_error() noexcept { return std::make_error_code(std::errc{errno}); }
	inline static std::uint64_t page_size() noexcept
	{
		const auto res = sysconf(_SC_PAGE_SIZE);
		return res < 0 ? SEK_KB(8) : static_cast<std::uint64_t>(res);
	}
}	 // namespace sek::detail