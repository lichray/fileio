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

#ifndef _STDEX_FILE_H
#define _STDEX_FILE_H

#include "traits_adaptors.h"
#include "polymorphic_allocator.h"
#include "lock_guard.h"
#include "charmap.h"

#include <memory>
#include <system_error>
#include <locale>
#include <string>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <io.h>
#endif

namespace stdex
{
#if !defined(_WIN32)
#define _isatty ::isatty
#define _lock_file ::flockfile
#define _unlock_file ::funlockfile
#endif

#if defined(fileno)
#error fix your libc by guarding the `fileno` macro against __cplusplus
#endif

namespace pmr = xstd::polyalloc;

using std::error_code;
using std::nullptr_t;
using std::allocator_arg_t;
using std::allocator_arg;
using xstd::erased_type;

enum class whence
{
	beginning = SEEK_SET,
	current = SEEK_CUR,
	ending = SEEK_END,
};

enum class opening
{
	fully_buffered = 0x0001,
	line_buffered = 0x0002,
	buffered = fully_buffered | line_buffered,
	for_read = 0x0004,
	for_write = 0x0008,
	append_mode = 0x0010,
};

template <typename Enum>
struct _ifflags
{
	using int_type = std::underlying_type_t<Enum>;

	_ifflags(Enum e) noexcept : v_(int_type(e))
	{}

	friend
	_ifflags operator|(_ifflags a, _ifflags b) noexcept
	{
		return _ifflags(a.v_ | b.v_);
	}

	explicit operator int_type() noexcept
	{
		return v_;
	}

private:
	explicit _ifflags(int_type v) noexcept : v_(v)
	{}

	int_type v_;
};

inline
auto operator|(opening a, opening b) noexcept
{
	return _ifflags<opening>(a) | _ifflags<opening>(b);
}

struct file
{
	using off_t = int_least64_t;
	using allocator_type = erased_type;

	struct io_result
	{
		io_result() noexcept :
			ok_(), n_()
		{}

		io_result(bool ok, size_t n) noexcept :
			ok_(ok), n_(n)
		{}

		explicit operator bool() const noexcept
		{
			return ok_;
		}

		size_t count() const noexcept
		{
			return n_;
		}

	private:
		bool ok_;
		size_t n_;
	};

private:
	template <typename T>
	using being_readable = decltype(std::declval<int&>() =
	    std::declval<T&>().read((char*){}, int()));

	template <typename T>
	using being_writable = decltype(std::declval<int&>() =
	    std::declval<T&>().write((char const*){}, int()));

	template <typename T>
	using being_seekable = decltype(std::declval<off_t&>() =
	    std::declval<T&>().seek(off_t(), whence()));

	template <typename T>
	using being_closable = decltype(std::declval<int&>() =
	    std::declval<T&>().close());

	template <typename T>
	using being_resizable = decltype(std::declval<int&>() =
	    std::declval<T&>().resize(off_t()));

	template <typename T>
	using is_readable = detector_of<being_readable>::template call<T>;

	template <typename T>
	using is_writable = detector_of<being_writable>::template call<T>;

	template <typename T>
	using is_seekable = detector_of<being_seekable>::template call<T>;

	template <typename T>
	using is_closable = detector_of<being_closable>::template call<T>;

	template <typename T>
	using is_resizable = detector_of<being_resizable>::template call<T>;

#if defined(WIN32)
	using ssize_t = ptrdiff_t;
#endif
	using _unspecified_ = _ifflags<opening>;

public:
	file() noexcept :
		file(allocator_arg, nullptr)
	{}

	file(allocator_arg_t, nullptr_t) noexcept :
		mr_p_(pmr::get_default_resource())
	{}

	file(allocator_arg_t, pmr::memory_resource* p) noexcept :
		mr_p_(p)
	{}

	template <typename T, typename =
	    If<either<is_readable, is_writable>::call<T>>>
	file(T&& t, _unspecified_ flags) :
		file(allocator_arg, nullptr, std::forward<T>(t), flags)
	{}

	template <typename T, typename =
	    If<either<is_readable, is_writable>::call<T>>>
	file(T&& t, _unspecified_ flags, int bufsize) :
		file(allocator_arg, nullptr, std::forward<T>(t), flags, bufsize)
	{}

	template <typename T, typename Alloc, typename =
	    If<either<is_readable, is_writable>::call<T>>>
	file(allocator_arg_t, Alloc const& alloc, T&& t, _unspecified_ flags,
	    int bufsize = 0) :
		file(allocator_arg, alloc)
	{
		static_assert(std::is_same<decltype(get_fd(t)), int>(),
		    "fd() must return int if presents");
		fd_copy_ = get_fd(t);

		pmr::polymorphic_allocator<io_core<T>> a(mr_p_);
		auto p = a.allocate(1);
		a.construct(p, std::forward<T>(t));
		fp_.reset(p);
		assert(opened());

		flags_ = decltype(flags_)(flags);
		if (!is_readable<T>())
			make_it_not(for_read);
		if (!is_writable<T>())
			make_it_not(for_write);
		if (!is_seekable<T>())
			make_it_not(append_mode);
		if (bufsize != 0 and not buffering())
			make_it(buffered);
		blen_ = bufsize;
	}

	file(file&&) noexcept = default;

	file& operator=(file&& other) noexcept
	{
		_destruct();
		xp_ = std::move(other.xp_);
		fp_ = std::move(other.fp_);
		bp_ = std::move(other.bp_);
		// the followings are OK to be copied
		r_ = std::move(other.r_);
		w_ = std::move(other.w_);
		p_ = std::move(other.p_);
		blen_ = std::move(other.blen_);
		flags_ = std::move(other.flags_);
		fd_copy_ = std::move(other.fd_copy_);
		mr_p_ = std::move(other.mr_p_);

		return *this;
	}

	friend
	void swap(file& lhs, file& rhs) noexcept
	{
		using std::swap;
		swap(lhs.xp_, rhs.xp_);
		swap(lhs.fp_, rhs.fp_);
		swap(lhs.bp_, rhs.bp_);
		swap(lhs.r_, rhs.r_);
		swap(lhs.w_, rhs.w_);
		swap(lhs.p_, rhs.p_);
		swap(lhs.blen_, rhs.blen_);
		swap(lhs.flags_, rhs.flags_);
		swap(lhs.fd_copy_, rhs.fd_copy_);
		swap(lhs.mr_p_, rhs.mr_p_);
	}

	FILE* locking(nullptr_t)
	{
		assert(opened());
		return xp_.release();
	}

	FILE* locking(FILE* stream)
	{
		assert(opened());
		auto p = xp_.get();
		xp_.reset(stream);
		return p;
	}

	bool readable() const noexcept
	{
		auto _ = make_guard();
		return it_is(for_read);
	}

	bool writable() const noexcept
	{
		auto _ = make_guard();
		return it_is(for_write);
	}

	bool isatty() const
	{
		return fileno() != -1 && _isatty(fileno());
	}

	int fileno() const
	{
		assert(opened());
		return fd_copy_;
	}

	bool closed() const noexcept
	{
		auto _ = make_guard();
		return it_is_not(for_read | for_write);
	}

	io_result read(char* buf, size_t sz)
	{
		error_code ec;
		auto r = read(buf, sz, ec);
		if (ec) throw std::system_error(ec);

		return r;
	}

	io_result write(char const* buf, size_t sz)
	{
		error_code ec;
		auto r = write(buf, sz, ec);
		if (ec) throw std::system_error(ec);

		return r;
	}

	io_result put(char c)
	{
		assert(opened());
		auto _ = make_guard();

		if (auto r = put_fasttrack(c))
			return r;

		error_code ec;
		auto r = put_nolock(c, ec);
		if (ec) throw std::system_error(ec);

		return r;
	}

	off_t seek(off_t offset, whence where)
	{
		error_code ec;
		auto off = seek(offset, where, ec);
		if (ec) throw std::system_error(ec);

		return off;
	}

	void rewind()
	{
		seek(0, whence::beginning);
	}

	off_t tell()
	{
		error_code ec;
		auto off = tell(ec);
		if (ec) throw std::system_error(ec);

		return off;
	}

	void resize(off_t len)
	{
		error_code ec;
		resize(len, ec);
		if (ec) throw std::system_error(ec);
	}

	void truncate()
	{
		resize(tell());
	}

	void flush()
	{
		error_code ec;
		flush(ec);
		if (ec) throw std::system_error(ec);
	}

	void close()
	{
		error_code ec;
		close(ec);
		if (ec) throw std::system_error(ec);
	}

	io_result read(char* buf, size_t sz, error_code& ec)
	{
		auto n = fp_->read(buf, int(sz));

		if (n == -1)
		{
			ec.assign(errno, std::system_category());
			return {};
		}

		return { size_t(n) == sz, size_t(n) };
	}

	io_result write(char const* buf, size_t sz, error_code& ec)
	{
		assert(opened());
		auto _ = make_guard();
		return write_nolock(buf, sz, ec);
	}

	io_result put(char c, error_code& ec)
	{
		assert(opened());
		auto _ = make_guard();

		if (auto r = put_fasttrack(c))
			return r;

		return put_nolock(c, ec);
	}

	off_t seek(off_t offset, whence where, error_code& ec)
	{
		auto off = fp_->seek(offset, where);

		if (off == -1)
			ec.assign(errno, std::system_category());

		return off;
	}

	void rewind(error_code& ec)
	{
		seek(0, whence::beginning, ec);
	}

	off_t tell(error_code& ec)
	{
		return seek(0, whence::current, ec);
	}

	void resize(off_t len, error_code& ec)
	{
		auto r = fp_->resize(len);

		if (r == -1)
			ec.assign(errno, std::system_category());
	}

	void truncate(error_code& ec)
	{
		error_code ec2;
		auto off = tell(ec2);

		if (ec2)
			ec = ec2;
		else
			resize(off, ec);
	}

	void flush(error_code& ec)
	{
		assert(opened());
		auto _ = make_guard();
		flush_nolock(ec);
	}

	void close(error_code& ec)
	{
		assert(opened());
		auto _ = make_guard();
		close_nolock(ec);
	}

	~file()
	{
		auto _ = make_guard();
		_destruct();
	}

private:
	enum flag
	{
		// if neither presents, unbuffered
		fully_buffered = int(opening::fully_buffered),
		line_buffered = int(opening::line_buffered),
		// meaning TBD
		buffered = int(opening::buffered),
		// if both present, opened for read and write
		for_read = int(opening::for_read),
		for_write = int(opening::for_write),
		append_mode = int(opening::append_mode),
		// other states
		reached_eof = 0x0100,
		reading = 0x1000,
		writing = 0x2000,
	};

	bool it_is(int v) const
	{
		return flags_ & v;
	}

	bool it_is_not(int v) const
	{
		return !it_is(v);
	}

	void make_it(int v)
	{
		flags_ |= v;
	}

	void make_it_not(int v)
	{
		flags_ &= ~v;
	}

	struct io_interface
	{
		virtual int read(char* buf, int n) = 0;
		virtual int write(char const* buf, int n) = 0;
		virtual off_t seek(off_t offset, whence where) = 0;
		virtual int close() noexcept = 0;
		virtual int resize(off_t len) = 0;

		virtual void delete_with(pmr::memory_resource*) noexcept = 0;
	};

	template <typename T>
	struct io_core final : io_interface
	{
		using allocator_type = pmr::polymorphic_allocator<io_core<T>>;

		io_core(T const& rep, allocator_type const& a) :
			rep_(allocator_arg, a, rep)
		{}

		io_core(T&& rep, allocator_type const& a) :
			rep_(allocator_arg, a, std::move(rep))
		{}

		int read(char* buf, int n) override
		{
			return read(buf, n, is_readable<T>());
		}

		int write(char const* buf, int n) override
		{
			return write(buf, n, is_writable<T>());
		}

		off_t seek(off_t offset, whence where) override
		{
			return seek(offset, where, is_seekable<T>());
		}

		int close() noexcept override
		{
			return close(is_closable<T>());
		}

		int resize(off_t len) override
		{
			return resize(len, is_resizable<T>());
		}

		void delete_with(pmr::memory_resource* mr_p) noexcept override
		{
			pmr::polymorphic_allocator<io_core> a(mr_p);
			a.destroy(this);
			a.deallocate(this, 1);
		}

	private:
		int read(char* buf, int n, std::true_type)
		{
			return obj().read(buf, n);
		}

		int read(char*, int, std::false_type)
		{
			return -1;
		}

		int write(char const* buf, int n, std::true_type)
		{
			return obj().write(buf, n);
		}

		int write(char const*, int, std::false_type)
		{
			return -1;
		}

		off_t seek(off_t offset, whence where, std::true_type)
		{
			return obj().seek(offset, where);
		}

		off_t seek(off_t offset, whence where, std::false_type)
		{
			return -1;
		}

		int close(std::true_type) noexcept
		{
			return obj().close();
		}

		int close(std::false_type) noexcept
		{
			return 0;
		}

		int resize(off_t len, std::true_type)
		{
			return obj().resize(len);
		}

		int resize(off_t len, std::false_type)
		{
			return -1;
		}

	private:
		T& obj() noexcept
		{
			return rep_.value();
		}

		xstd::uses_allocator_construction_wrapper<T> rep_;
	};

	friend struct std_streams_resource;

	template <typename T>
	static auto get_fd(T const& t) -> decltype(t.fd())
	{
		return t.fd();
	}

	static int get_fd(...)
	{
		return -1;
	}

	static void report_error(error_code& ec, int eno)
	{
		ec.assign(eno, std::system_category());
	}

	bool opened() const noexcept
	{
		return (bool)fp_;
	}

	flag buffering() const
	{
		return flag(flags_ & buffered);
	}

	bool buffer_clear() const
	{
		return bp_.get() == p_;
	}

	bool fits_in_buffer(size_t n) const
	{
		return space_left() >= n;
	}

	int buffer_use() const
	{
		return int(p_ - bp_.get());
	}

	size_t space_left() const
	{
		return blen_ - buffer_use();
	}

	void prepare_to_write()
	{
		if (it_is(reading))
		{
			make_it_not(reading | reached_eof);
			p_ = bp_.get();
			w_ = blen_;
		}
		make_it(writing);

		if (buffering() && bp_ == nullptr)
			setup_buffer();
	}

	void seek_if_appending()
	{
		if (it_is(append_mode))
			(void)fp_->seek(0, whence::ending);
	}

	static constexpr int default_buffer_size = 8192;
	static constexpr auto buffer_alignment =
#if !defined(_WIN32)
	    alignof(char);
#else
	    alignof(wchar_t);
#endif

	void setup_buffer();

	void copy_to_buffer(char const* p, size_t sz, size_t& written)
	{
		assert(fits_in_buffer(sz));
		memmove(p_, p, sz);
		p_ += sz;
		written += sz;
	}

	bool sflush();
	bool swrite(char const* p, size_t sz, size_t& written);
	bool swrite_b(char const* p, size_t sz, size_t& written);
	bool sclose();

	io_result write_nolock(char const* buf, size_t sz, error_code& ec);
	io_result put_nolock(char c, error_code& ec);

	io_result put_fasttrack(char c)
	{
		if (--w_ >= 0 &&
		    (c != charmap<char>::eol || buffering() != line_buffered))
		{
			*p_++ = c;
			return { true, 1 };
		}

		return {};
	}

	void flush_nolock(error_code& ec)
	{
		if (!sflush())
			report_error(ec, errno);
	}

	void close_nolock(error_code& ec)
	{
		if (!sclose())
			report_error(ec, errno);
	}

	void _destruct() noexcept
	{
		if (opened())
		{
			(void)sclose();
			fp_.release()->delete_with(mr_p_);
		}
	}

	struct noop_deleter
	{
		template <typename T>
		void operator()(T*)
		{}
	};

	void lock() const
	{
		_lock_file(xp_.get());
	}

	void unlock() const
	{
		_unlock_file(xp_.get());
	}

	using lock_guard = conditional_lock_guard<file const>;
	friend lock_guard;

	lock_guard make_guard() const
	{
		return { (bool)xp_, *this };
	}

	std::unique_ptr<FILE, noop_deleter> xp_;
	std::unique_ptr<io_interface, noop_deleter> fp_;
	std::unique_ptr<char[], noop_deleter> bp_;
	int r_ = 0;
	int w_ = 0;
	char* p_ = nullptr;
	int blen_;
	_ifflags<opening>::int_type flags_;
	int fd_copy_;
	pmr::memory_resource* mr_p_;
	//mbstate_t mbs_{};
};

#undef _isatty
#undef _lock_file
#undef _unlock_file
}

#endif
