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

#ifndef _STDEX_STRING_ALGO_H
#define _STDEX_STRING_ALGO_H

#include <algorithm>
#include <iterator>

namespace stdex
{

template <typename From, typename To>
using copy_const_t = std::conditional_t<std::is_const<From>{},
    std::add_const_t<To>, To>;

template <typename CharT, typename R = copy_const_t<CharT, char>*>
auto as_bytes(CharT& c) -> R
{
	return reinterpret_cast<R>(std::addressof(c));
}

template <typename CharT, typename R = copy_const_t<CharT, char>*>
auto as_bytes(CharT* s) -> R
{
	return reinterpret_cast<R>(s);
}

template <typename CharT>
inline
CharT const* rfind(CharT const* s, size_t n, CharT c)
{
	using Iter = std::reverse_iterator<decltype(s)>;
	auto it = std::find(Iter(s + n), Iter(s), c);
	return it.base();
}

}

#endif
