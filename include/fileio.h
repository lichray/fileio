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

#ifndef _STDEX_FILEIO_H
#define _STDEX_FILEIO_H

#include "traits_adaptors.h"

#include <mutex>
#include <memory>
#include <system_error>
#include <wchar.h>

#include <unistd.h>

namespace stdex
{

struct file
{
private:
	struct io_interface;

public:
	using off_t = int_least64_t;
	using erasure_type = std::unique_ptr<io_interface>;

	enum class seekdir
	{};

	enum class buffering_behavior
	{};

	struct io_result
	{
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
	    std::declval<T&>().seek(off_t(), seekdir()));

	template <typename T>
	using being_closable = decltype(std::declval<int&>() =
	    std::declval<T&>().close());

	template <typename T>
	using is_readable = detector_of<being_readable>::template call<T>;

	template <typename T>
	using is_writable = detector_of<being_writable>::template call<T>;

	template <typename T>
	using is_seekable = detector_of<being_seekable>::template call<T>;

	template <typename T>
	using is_closable = detector_of<being_closable>::template call<T>;

public:
	file() noexcept : mbs_()
	{}

	template <typename T, typename =
	    If<either<is_readable, is_writable>::call<T>>>
	explicit file(T&& t) :
		file(std::make_unique<io_core<T>>(std::forward<T>(t)))
	{}

	explicit file(erasure_type fp) noexcept :
		fp_(std::move(fp)), mbs_()
	{}

	static constexpr auto fully_buffered = buffering_behavior(0);
	static constexpr auto line_buffered = buffering_behavior(1);
	static constexpr auto unbuffered = buffering_behavior(2);

	static constexpr auto beginning = seekdir(SEEK_SET);
	static constexpr auto current = seekdir(SEEK_CUR);
	static constexpr auto ending = seekdir(SEEK_END);

	io_result read(char* buf, size_t sz)
	{
		auto n = fp_->read(buf, sz);
		return { size_t(n) == sz, size_t(n) };
	}

	io_result write(char const* buf, size_t sz)
	{
		auto n = fp_->write(buf, sz);
		return { size_t(n) == sz, size_t(n) };
	}

	off_t seek(off_t offset, seekdir where)
	{
		auto off = fp_->seek(offset, where);
		return off;
	}

	void close()
	{
		fp_->close();
	}

	void setbuf(buffering_behavior mode)
	{
	}

	erasure_type detach()
	{
		return std::move(fp_);
	}

	void lock() const
	{
		mu_.lock();
	}

	bool try_lock() const
	{
		return mu_.try_lock();
	}

	void unlock() const
	{
		mu_.unlock();
	}

	~file()
	{
		if (try_lock())
		{
			lock_guard _{*this, std::adopt_lock};
			if (fp_)
				(void)fp_->close();
		}
	}

private:
	struct io_interface
	{
		virtual int read(char* buf, int n) = 0;
		virtual int write(char const* buf, int n) = 0;
		virtual off_t seek(off_t offset, seekdir where) = 0;
		virtual int close() noexcept = 0;

		virtual ~io_interface()
		{}
	};

	template <typename T>
	struct io_core final : io_interface
	{
		explicit io_core(T rep) : rep_(std::move(rep))
		{}

		int read(char* buf, int n) override
		{
			return read(buf, n, is_readable<T>());
		}

		int write(char const* buf, int n) override
		{
			return write(buf, n, is_writable<T>());
		}

		off_t seek(off_t offset, seekdir where) override
		{
			return seek(offset, where, is_seekable<T>());
		}

		int close() noexcept override
		{
			return close(is_closable<T>());
		}

	private:
		int read(char* buf, int n, std::true_type)
		{
			return rep_.read(buf, n);
		}

		int read(char*, int, std::false_type)
		{
			return -1;
		}

		int write(char const* buf, int n, std::true_type)
		{
			return rep_.write(buf, n);
		}

		int write(char const*, int, std::false_type)
		{
			return -1;
		}

		off_t seek(off_t offset, seekdir where, std::true_type)
		{
			return rep_.seek(offset, where);
		}

		off_t seek(off_t offset, seekdir where, std::false_type)
		{
			return -1;
		}

		int close(std::true_type) noexcept
		{
			return rep_.close();
		}

		int close(std::false_type) noexcept
		{
			return 0;
		}

		T rep_;
	};

	using lock_guard = std::lock_guard<file>;

	mutable std::recursive_mutex mu_;
	erasure_type fp_;
	std::unique_ptr<char[]> bp_;
	int blen_;
	mbstate_t mbs_;
};

}

#endif
