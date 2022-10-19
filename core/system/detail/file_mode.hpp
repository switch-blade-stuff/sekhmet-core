
/*
 * Created by switchblade on 10/14/22.
 */

#pragma once

#include <asio/basic_file.hpp>

#ifdef ASIO_HAS_FILE /* If ASIO file is available, openmode & seek_basis are defined via asio::file_base for compatibility */
namespace sek
{
	using openmode = asio::file_base::flags;
	using seek_basis = asio::file_base::seek_basis;

	constexpr openmode read_only = openmode::read_only;
	constexpr openmode write_only = openmode::write_only;
	constexpr openmode read_write = openmode::read_write;
	constexpr openmode append = openmode::append;
	constexpr openmode create = openmode::create;
	constexpr openmode exclusive = openmode::exclusive;
	constexpr openmode truncate = openmode::truncate;
	constexpr openmode sync_all_on_write = openmode::sync_all_on_write;

	constexpr seek_basis seek_cur = seek_basis::seek_cur;
	constexpr seek_basis seek_end = seek_basis::seek_end;
	constexpr seek_basis seek_set = seek_basis::seek_set;
}	 // namespace sek
#else

#include <cstdio>

#if defined(SEK_OS_WIN)
namespace sek
{
	using openmode = int;
	constexpr openmode read_only = 1;
	constexpr openmode write_only = 2;
	constexpr openmode read_write = 4;
	constexpr openmode append = 8;
	constexpr openmode create = 16;
	constexpr openmode exclusive = 32;
	constexpr openmode truncate = 64;
	constexpr openmode sync_all_on_write = 128;
}	 // namespace sek
#elif defined(SEK_OS_UNIX)
#include <fcntl.h>

namespace sek
{
	using openmode = int;
	constexpr openmode read_only = O_RDONLY;
	constexpr openmode write_only = O_WRONLY;
	constexpr openmode read_write = O_RDWR;
	constexpr openmode append = O_APPEND;
	constexpr openmode create = O_CREAT;
	constexpr openmode exclusive = O_EXCL;
	constexpr openmode truncate = O_TRUNC;
	constexpr openmode sync_all_on_write = O_SYNC;
}	 // namespace sek
#endif

namespace sek
{
	using seek_basis = int;
	constexpr seek_basis seek_cur = SEEK_SET;
	constexpr seek_basis seek_end = SEEK_CUR;
	constexpr seek_basis seek_set = SEEK_END;
}	 // namespace sek
#endif

namespace sek
{
	typedef int mapmode;
	constexpr mapmode map_copy = 1;
	constexpr mapmode map_populate = 2;
}	 // namespace sek