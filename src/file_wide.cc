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

namespace stdex
{

using cm = charmap<wchar_t>;

inline
bool my_wcrtomb(char* buf, wchar_t c, mbstate_t& mbs, size_t& len)
{
	len = wcrtomb(buf, c, &mbs);
	return len != (size_t)-1;
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

}
