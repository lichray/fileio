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

#if !defined(_WIN32)
#include <sys/param.h>
#include <unistd.h>
#else
#include <io.h>
#endif

namespace stdex
{
#if !defined(_WIN32)
#define _read ::read
#define _write ::write
#define _close ::close
#if defined(BSD) || defined(__MSYS__)
#define _lseeki64 ::lseek
#define _chsizei64 ::ftruncate
#else
#define _lseeki64 ::lseek64
#define _chsizei64 ::ftruncate64
#endif
#endif

namespace detail
{

template <typename Rt = void, typename F, typename... Args>
inline
auto syscall(F f, Args... args)
{
	using R = void_or<Rt, std::result_of<F(Args...)>>;
#if defined(BSD) || defined(__MSYS__) || defined(_WIN32)
	return R(f(args...));
#else
	R r;
	do
	{
		r = R(f(args...));
	} while (r == -1 && errno == EINTR);

	return r;
#endif
}

#if !(defined(BSD) || defined(__MSYS__) || defined(_WIN32))

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
	using native_handle_type = int;

	explicit file_stream(native_handle_type fd) : fd_(fd)
	{}

	int read(char* buf, int n)
	{
		return detail::syscall<int>(_read, fd_, buf, n);
	}

	int write(char const* buf, int n)
	{
		return detail::syscall<int>(_write, fd_, buf, n);
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
#if defined(_WIN32)
		return _chsize_s(fd_, len) == 0 ? 0 : -1;
#else
		return detail::syscall(_chsizei64, fd_, len);
#endif
	}

	// optional if native_handle_type != int
	int fd() const noexcept
	{
		return fd_;
	}

private:
	native_handle_type fd_;
};

template <typename CharT>
file allocate_file(pmr::memory_resource* mrp, CharT const* path,
    char const* mode, error_code& ec);

template <typename CharT>
inline
file allocate_file(pmr::memory_resource* mrp, CharT const* path,
    char const* mode)
{
	error_code ec;
	auto fh = allocate_file(mrp, path, mode, ec);
	if (ec) throw std::system_error(ec);

	return fh;
}

template <typename CharT, typename Traits, typename Alloc>
inline
file allocate_file(pmr::memory_resource* mrp,
    std::basic_string<CharT, Traits, Alloc> const& path, char const* mode,
    error_code& ec)
{
	return allocate_file(mrp, path.data(), mode, ec);
}

template <typename CharT, typename Traits, typename Alloc>
inline
file allocate_file(pmr::memory_resource* mrp,
    std::basic_string<CharT, Traits, Alloc> const& path, char const* mode)
{
	return allocate_file(mrp, path.data(), mode);
}

template <typename CharT>
inline
file open_file(CharT const* path, char const* mode, error_code& ec)
{
	return allocate_file(pmr::get_default_resource(), path, mode, ec);
}

template <typename CharT>
inline
file open_file(CharT const* path, char const* mode)
{
	return allocate_file(pmr::get_default_resource(), path, mode);
}

template <typename CharT, typename Traits, typename Alloc>
inline
file open_file(std::basic_string<CharT, Traits, Alloc> const& path,
    char const* mode, error_code& ec)
{
	return open_file(path.data(), mode, ec);
}

template <typename CharT, typename Traits, typename Alloc>
inline
file open_file(std::basic_string<CharT, Traits, Alloc> const& path,
    char const* mode)
{
	return open_file(path.data(), mode);
}

#undef _read
#undef _write
#undef _close
#undef _lseeki64
#undef _chsizei64
}

#endif
