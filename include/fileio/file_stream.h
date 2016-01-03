/*-
 * Copyright (c) 2015 Zhihao Yuan.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _STDEX_FILE_STREAM_H
#define _STDEX_FILE_STREAM_H

#include "file.h"

#if !defined(WIN32)
#include <sys/param.h>
#endif

namespace stdex
{
#if !defined(WIN32)
#define _read ::read
#define _write ::write
#define _close ::close
#if defined(BSD)
#define _lseeki64 ::lseek
#define _chsizei64 ::ftruncate
#else
#define _lseeki64 ::lseek64
#define _chsizei64 ::ftruncate64
#endif
#endif

namespace detail
{

template <typename F, typename... Args>
inline
auto syscall(F f, Args... args)
{
#if defined(BSD) || defined(WIN32)
	return f(args...);
#else
	decltype(f(args...)) r;
	do
	{
		r = f(args...);
	} while (r == -1 && errno == EINTR);

	return r;
#endif
}

#if !defined(BSD) && !defined(WIN32)

inline
auto syscall(decltype(&_close) f, int fd)
{
	int r = f(fd);
	if (r == -1 && errno == EINTR)
		return 0;

	return r;
}

#endif
}

struct file_stream
{
	using native_handle = int;

	explicit file_stream(native_handle fd) : fd_(fd)
	{}

	int read(char* buf, int n)
	{
		return detail::syscall(_read, fd_, buf, n);
	}

	int write(char const* buf, int n)
	{
		return detail::syscall(_write, fd_, buf, n);
	}

	file::off_t seek(file::off_t offset, whence where)
	{
		return detail::syscall(_lseeki64, fd_, offset, int(where));
	}

	int close() noexcept
	{
		return detail::syscall(_close, fd_);
	}

	int resize(file::off_t len)
	{
#if defined(WIN32)
		return _chsize_s(fd_, len) == 0 ? 0 : -1;
#else
		return detail::syscall(_chsizei64, fd_, len);
#endif
	}

private:
	native_handle fd_;
};

#undef _read
#undef _write
#undef _close
#undef _lseeki64
#undef _chsizei64

}

#endif
