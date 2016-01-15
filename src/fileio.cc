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

#include <fileio.h>

namespace stdex
{

struct std_streams_resource : pmr::memory_resource
{
	void* allocate(size_t bytes, size_t alignment) override
	{
		if (alignment == file::buffer_alignment)
			return up_->allocate(bytes, alignment);
		else
		{
			assert(i_ < 3 && "no more standard streams please");
			return &a_[i_++];
		}
	}

	void deallocate(void* p, size_t bytes, size_t alignment) override
	{
		if (alignment == file::buffer_alignment)
			up_->deallocate(p, bytes, alignment);
	}

	bool is_equal(memory_resource const&) const override
	{
		return false;
	}

private:
	using target_t = file::io_core<file_stream>;
	static_assert(alignof(target_t) > file::buffer_alignment, "sorry");

	int i_ = 0;
	pmr::memory_resource* up_ = pmr::get_default_resource();
	std::aligned_storage_t<sizeof(target_t), alignof(target_t)> a_[3];
};

static std_streams_resource __a;

template <typename Flags>
inline
file make_std_stream(int fd, Flags mode, FILE* target)
{
	file fh(allocator_arg, &__a, file_stream(fd), mode);
	fh.locking(target);

	return fh;
}

file in = make_std_stream(0, opening::buffered | opening::for_read, stdin);
file out = make_std_stream(1, opening::buffered | opening::for_write, stdout);
file err = make_std_stream(2, opening::for_write, stderr);

}
