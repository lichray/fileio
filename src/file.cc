/*-
 * Copyright (c) 2016 Zhihao Yuan.  All rights reserved.
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

#define NOMINMAX

#include <fileio/file.h>

#include "char_traits.h"

#include <iterator>
#include <ciso646>
#include <limits.h>

#if !defined(_WIN32)
#include <sys/param.h>
#endif
#include <sys/stat.h>

#if !defined(_WIN32)
#define _isatty ::isatty
#if defined(BSD) || defined(__MSYS__)
#define _stat64 ::stat
#define _fstat64 ::fstat
#else
#define _stat64 ::stat64
#define _fstat64 ::fstat64
#endif
#endif

using std::next;
using std::distance;

namespace stdex
{

file::io_result file::write_nolock(char const* buf, size_t sz, error_code& ec)
{
	using ct = char_traits<char>;

	if (sz == 0)
		return { true, 0 };

	if (not writable())
	{
		report_error(ec, EBADF);
		return {};
	}

	prepare_to_write();
	size_t written = 0;
	bool ok;

	switch (buffering())
	{
	case fully_buffered:
		ok = swrite_b(buf, sz, written);
		break;
	case line_buffered:
		if (buffer_clear() and buf[sz - 1] == ct::eol)
			ok = swrite(buf, sz, written);
		else
		{
			auto ep = rfind(buf, sz, ct::eol);
			if (ep == nullptr)
				ok = swrite_b(buf, sz, written);
			else
			{
				auto d = distance(buf, ep);
				if (not buffer_clear() and fits_in_buffer(d))
				{
					copy_to_buffer(buf, d, written);
					ok = sflush();
				}
				else
				{
					// not trying to fill the buffer
					ok = sflush();
					if (ok) ok = swrite(buf, d, written);
				}
				if (ok) ok = swrite_b(ep, sz - d, written);
			}
		}
		break;
	default:
		ok = swrite(buf, sz, written);
	}

	if (not ok)
		report_error(ec, errno);

	return { ok, written };
}

void file::prepare_buffer()
{
	if (bp_ == nullptr)
	{
		// Windows has no st_blksize, MSYS2 sets erroneous st_blksize
#if defined(_WIN32) || defined(__MSYS__)
		if (blen_ == 0)
			blen_ = default_buffer_size;
		if (buffering() == buffered)
		{
			if (isatty())
				make_it_not(fully_buffered);
			else
				make_it_not(line_buffered);
		}
#else
		struct _stat64 st;
		if (fd_copy_ == -1 or _fstat64(fd_copy_, &st) == -1)
		{
			if (blen_ == 0)
				blen_ = default_buffer_size;
			if (buffering() == buffered)
				make_it_not(line_buffered);
		}
		else
		{
			if (blen_ == 0)
				blen_ = st.st_blksize <= 0 ?
				    default_buffer_size :
				    st.st_blksize;
			if (buffering() == buffered)
			{
				if (S_ISCHR(st.st_mode) and _isatty(fd_copy_))
					make_it_not(fully_buffered);
				else
					make_it_not(line_buffered);
			}
		}
#endif
		bp_.reset((char*)mr_p_->allocate(blen_));
		p_ = bp_.get();
	}
}

bool file::swrite(char const*p, size_t sz, size_t& written)
{
	int n;
#if defined(_WIN32)
	if (sz > 32767 and isatty())
		n = 32767;
#endif
	if (sz > INT_MAX)
		n = INT_MAX;
	else
		n = int(sz);

	while (sz != 0)
	{
		auto r = fp_->write(p, n);
		if (r == -1)
		{
			make_it(in_error);
			return false;
		}
		p += r;
		sz -= r;
		written += r;
		if (sz < size_t(n))
			n = int(sz);
	}

	return true;
}

bool file::swrite_b(char const*p, size_t sz, size_t& written)
{
	bool ok = true;

	while (ok and sz != 0)
	{
		auto m = std::min(space_left(), sz);

		// buffer is full
		if (m == 0)
		{
			ok = sflush();
		}
		// buffer empty and we have a chunk to write
		else if (m == size_t(blen_))
		{
			ok = swrite(p, m, written);
			p += m;
			sz -= m;
		}
		else
		{
			copy_to_buffer(p, m, written);
			p += m;
			sz -= m;
		}
	}

	if (not ok)
		make_it(in_error);

	return ok;
}

bool file::sflush()
{
	int sz = buffer_use();
	int n = sz;
#if defined(_WIN32)
	if (n > 32767 and isatty())
		n = 32767;
#endif
	auto p = bp_.get();
	while (sz != 0)
	{
		auto r = fp_->write(p, n);
		if (r == -1)
		{
			memmove(bp_.get(), p, sz);
			p_ = bp_.get() + sz;
			make_it(in_error);
			return false;
		}
		p += r;
		sz -= r;
		if (sz < n)
			n = sz;
	}

	p_ = bp_.get();
	return true;
}

bool file::sclose()
{
	if (it_is(closed_))
		return true;

	bool flushok = it_is(writing) ? sflush() : true;
	auto eno = errno;
	make_it(closed_);

	bool closeok = (fp_->close() == 0);
	if (closeok and not flushok)
		errno = eno;
	return flushok and closeok;
}

}
