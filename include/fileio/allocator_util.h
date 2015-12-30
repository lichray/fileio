/* allocator_util.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_ALLOCATOR_UTIL_DOT_H
#define INCLUDED_ALLOCATOR_UTIL_DOT_H

#ifndef INCLUDED_ALLOCATOR_TRAITS_DOT_H
#include "allocator_traits.h"
#endif

BEGIN_NAMESPACE_XSTD

namespace __details {

// Class to hold both an allocator and another data object.
// Use inheritence to take advantage of the Empty Base Optimization (EBO).
template <typename _Alloc, typename _Tp>
class __allocator_wrapper : public _Alloc
{
    _Tp data_;

public:
    typedef _Alloc allocator_type;
    typedef _Tp    data_type;

    template <typename A, typename... Args>
    explicit __allocator_wrapper(A&& a, Args&&... args)
	: _Alloc(std::forward<A>(a)), data_(std::forward<Args>(args)...) { }

    _Tp      & data()       { return data_; }
    _Tp const& data() const { return data_; }

    _Alloc      & allocator()       { return *this; }
    _Alloc const& allocator() const { return *this; }

}; // end class __allocator_wrapper

} // end namespace __details

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_ALLOCATOR_UTIL_DOT_H)
