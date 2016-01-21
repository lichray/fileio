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

#include "string_algo.h"

#include <iterator>
#include <ciso646>
#include <limits.h>

using std::next;
using std::distance;

namespace stdex
{

using cm = charmap<wchar_t>;

static
bool my_wcrtomb(char* buf, wchar_t c, mbstate_t& mbs, size_t& len)
{
	len = wcrtomb(buf, c, &mbs);
	return len != (size_t)-1;
}

static
int my_wcsnrtombs(char* buf, wchar_t const* s, size_t& nwc, size_t blen,
    mbstate_t& mbs)
{
	auto p = buf;
	auto n = nwc;

#if !defined(_WIN32)
	auto bg = s;
	auto len = wcsnrtombs(p, &s, n, blen, &mbs);

	if (len == (size_t)-1)
		return -1;

	p += len;
	blen -= len;
	if (s == nullptr)  // restore it
		s = std::find(bg, next(bg, nwc), '\0');
	n -= distance(bg, s);
#endif

	while (n != 0 and blen >= cm::mb_len)
	{
		auto len = wcrtomb(p, *s++, &mbs);
		if (len == (size_t)-1)
			return -1;
		p += len;
		blen -= len;
		--n;
	}

	nwc = n;
	return int(p - buf);
}

void file::print_nolock(wchar_t const* s, size_t sz, error_code& ec)
{
	if (sz == 0)
		return;

	if (it_is_not(for_write))
	{
		report_error(ec, EBADF);
		return;
	}

	prepare_to_write();
	bool ok;

	switch (buffering())
	{
	case fully_buffered:
#if defined(_WIN32)
		if (bypass_wchar_conversion())
			ok = swrite_b(as_bytes(s), sz * cm::mb_len);
		else
#endif
			ok = swritew_b(s, sz);
		break;
	case line_buffered:
		{
			auto ep = rfind(s, sz, cm::eol);
			auto d = distance(s, ep);
#if defined(_WIN32)
			if (bypass_wchar_conversion())
			{
				auto before = d * cm::mb_len;
				auto after = (sz - d) * cm::mb_len;
				ok = swrite_b(as_bytes(s), before) and
				    sflush() and
				    swrite_b(as_bytes(ep), after);
			}
			else
#endif
				ok = swritew_b(s, d) and sflush() and
				    swritew_b(ep, sz - d);
		}
		break;
	default:
#if defined(_WIN32)
		if (bypass_wchar_conversion())
			ok = swrite(as_bytes(s), sz * cm::mb_len);
		else
#endif
		{
			char buf[default_buffer_size / 2];
			auto bp = buf;
			ok = xswritew(buf, sizeof(buf), bp, s, sz);

			if (ok and bp != buf)
				ok = swrite(buf, distance(buf, bp));
		}
	}

	if (not ok)
		report_error(ec, errno);
}

void file::print_nolock(wchar_t c, error_code& ec)
{
	if (it_is_not(for_write))
	{
		report_error(ec, EBADF);
		return;
	}

	prepare_to_write();
	bool ok = true;

	if (buffering())
	{
		if (not fits_in_buffer(cm::mb_len))
			ok = sflush();
		if (ok)
		{
#if defined(_WIN32)
			if (bypass_wchar_conversion())
			{
				*p_++ = as_bytes(c)[0];
				*p_++ = as_bytes(c)[1];
				w_ -= int(cm::mb_len);
			}
			else
#endif
			{
				size_t len;
				if ((ok = my_wcrtomb(p_, c, mbs_, len)))
				{
					p_ += len;
					w_ -= int(len);
				}
			}
			if (ok and c == cm::eol and
			    buffering() == line_buffered)
				ok = sflush();
		}
	}
	else
	{
#if defined(_WIN32)
		if (bypass_wchar_conversion())
			ok = swrite(as_bytes(c), cm::mb_len);
		else
#endif
		{
			char wcb[cm::mb_len];
			size_t len;
			if ((ok = my_wcrtomb(wcb, c, mbs_, len)))
				ok = swrite(wcb, len);
		}
	}

	if (not ok)
		report_error(ec, errno);
}

bool file::xswritew(char* buf, size_t blen, char*& bp, wchar_t const* p,
    size_t sz)
{
	seek_if_appending();

	auto d = int(bp - buf);
	auto n = sz;

	for (;;)
	{
		auto len = my_wcsnrtombs(bp, p + (sz - n), n, blen - d, mbs_);
		if (len == -1)
			return false;
		d += len;

		if (n == 0)
		{
			bp = buf + d;
			break;
		}

		auto r = fp_->write(buf, d);
		if (r == -1)
			return false;
		d -= r;
		memmove(buf, buf + r, d);
		bp = buf + d;
	}

	return true;
}

}
