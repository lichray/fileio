/* allocator_traits.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_ALLOCATOR_TRAITS_DOT_H
#define INCLUDED_ALLOCATOR_TRAITS_DOT_H

#include "xstd.h"
#include "memory_util.h"

#include <type_traits>

BEGIN_NAMESPACE_XSTD

// A class that uses type erasure to handle allocators should declare
// 'typedef erased_type allocator_type'
struct erased_type { };

namespace __details
{

template<typename _Tp, typename _Alloc>
struct __uses_allocator_imp
{
    // Use SFINAE (Substitution Failure Is Not An Error) to detect the
    // presence of an 'allocator_type' nested type convertilble from _Alloc.
    // Always return true if 'allocator_type' is 'erased_type'.

  private:
    template <typename _Up>
    static char __test(int, typename _Up::allocator_type);
        // __test is called twice, once with a second argument of type _Alloc
        // and once with a second argument of type erased_type.  Match this
        // overload if _TypeT::allocator_type exists and is implicitly
        // convertible from the second argument.

    template <typename _Up>
    static int __test(_LowPriorityConversion<int>,
		      _LowPriorityConversion<_Alloc>);
        // __test is called twice, once with a second argument of type _Alloc
        // and once with a second argument of type erased_type.  Match this
        // overload if called with a second argument of type _Alloc but
        // _TypeT::allocator_type does not exist or is not convertible from
        // _Alloc.

    template <typename _Up>
    static int __test(_LowPriorityConversion<int>,
		      _LowPriorityConversion<erased_type>);
        // __test is called twice, once with a second argument of type _Alloc
        // and once with a second argument of type erased_type.  Match this
        // overload if called with a second argument of type erased_type but
        // _TypeT::allocator_type does not exist or is not convertible from
        // erased_type.

  public:
    enum { value = (sizeof(__test<_Tp>(0, declval<_Alloc>())) == 1 ||
                    sizeof(__test<_Tp>(0, declval<erased_type>())) == 1) };
};

} // end namespace __details

template<typename _Tp, typename _Alloc>
struct uses_allocator
    : std::integral_constant<bool,
			__details::__uses_allocator_imp<_Tp, _Alloc>::value>
{
    // Automatically detects if there exists a type _Tp::allocator_type
    // that is convertible from _Alloc.  Specialize for types that
    // don't have a nested allocator_type but which are none-the-less
    // constructible with _Alloc (e.g., using type erasure).
};

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_ALLOCATOR_TRAITS_DOT_H)
