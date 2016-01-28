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

#define NOMINMAX

#include <fileio/file_stream.h>

#include <ciso646>
#include <iterator>

#include <fcntl.h>

#if defined(_WIN32)
#include <sys/stat.h>
#include <share.h>

static
int my_open(char const* path, int oflag)
{
	int fd;
	(void)_sopen_s(&fd, path, oflag | _O_NOINHERIT,
	    _SH_DENYRW, _S_IREAD | _S_IWRITE);

	return fd;
}

static
int my_open(wchar_t const* path, int oflag)
{
	int fd;
	(void)_wsopen_s(&fd, path, oflag | _O_NOINHERIT,
	    _SH_DENYRW, _S_IREAD | _S_IWRITE);

	return fd;
}

#else

#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR O_RDWR
#define _O_APPEND O_APPEND
#define _O_TRUNC O_TRUNC
#define _O_CREAT O_CREAT
#define _O_EXCL O_EXCL

static
int my_open(char const* path, int oflag)
{
#if defined(O_LARGEFILE)
	oflag |= O_LARGEFILE;
#endif
	return ::open(path, oflag | O_CLOEXEC, 0666);
}

#endif

using std::advance;

namespace stdex
{

static
void report_error(error_code& ec, int eno)
{
	ec.assign(eno, std::system_category());
}

template <typename CharT>
file allocate_file(pmr::memory_resource* mrp,
    CharT const* path, char const* mode, error_code& ec)
{
	int opts = int(opening::buffered);
	int flag = 0;

	switch (*mode++)
	{
	case 'r':
		opts |= int(opening::for_read);
		break;
	case 'w':
		opts |= int(opening::for_write);
		flag |= _O_CREAT | _O_TRUNC;
		break;
	case 'a':
		opts |= int(opening::for_write | opening::append_mode);
		flag |= _O_APPEND | _O_CREAT;
		break;
	case 'x':
		opts |= int(opening::for_write);
		flag |= _O_CREAT | _O_EXCL;
		break;
	default:
	invalid_mode:
		report_error(ec, EINVAL);
		return {};
	}

	if ((mode[0] == 'b' and mode[1] == '+') or
	    (mode[0] == '+' and mode[1] == 'b'))
	{
		opts |= int(opening::for_write | opening::for_read |
		    opening::binary);
		mode += 2;
	}
	else switch (*mode)
	{
	case 'b':
		opts |= int(opening::binary);
		++mode;
		break;
	case '+':
		opts |= int(opening::for_write | opening::for_read);
		++mode;
		break;
	}

#if defined(_WIN32)
	if (not (opts & int(opening::binary)))
	{
		if (*mode == ',')
		{
			while (*++mode == ' ');
			if (strncmp(mode, "ccs=", 4) != 0)
				goto invalid_mode;
			advance(mode, 4);
			if (_stricmp(mode, "unicode") == 0)
				flag |= _O_WTEXT;
			else if (_stricmp(mode, "utf-8") == 0)
				flag |= _O_U8TEXT;
			else if (_stricmp(mode, "utf-16le") == 0)
				flag |= _O_U16TEXT;
			else if (_stricmp(mode, "ansi") == 0)
				flag |= _O_TEXT;
			else
				goto invalid_mode;
		}
		else if (*mode != '\0')
			goto invalid_mode;
		// otherwise, global default _fmode
	}
	else
#endif
	if (*mode != '\0')
		goto invalid_mode;

	switch (opts & int(opening::for_write | opening::for_read))
	{
	case int(opening::for_read):
		flag |= _O_RDONLY;
		break;
	case int(opening::for_write):
		flag |= _O_WRONLY;
		break;
	default:
		flag |= _O_RDWR;
		break;
	}

	auto fd = my_open(path, flag);

	if (fd == -1)
	{
		report_error(ec, errno);
		return {};
	}

	return file(allocator_arg, mrp, file_stream(fd), opening(opts));
}

template file allocate_file(pmr::memory_resource* mrp,
    char const* path, char const* mode, error_code& ec);

#if defined(_WIN32)
template file allocate_file(pmr::memory_resource* mrp,
    wchar_t const* path, char const* mode, error_code& ec);
#endif

}
