/* uses_allocator_wrapper.h                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_USES_ALLOCATOR_WRAPPER_DOT_H
#define INCLUDED_USES_ALLOCATOR_WRAPPER_DOT_H

#include "allocator_traits.h"

#include <type_traits>

BEGIN_NAMESPACE_XSTD

using namespace std;

namespace __details {

template <class _Tp>
class __uses_allocator_construction_imp
{
    _Tp m_value;
public:
    // struct dont_care_type {
    //     // Type convertible from both true_type and false_type.
    //     // Used when you don't care
    //     dont_care_type(true_type) { }
    //     dont_care_type(false_type) { }
    // };
    typedef bool dont_care_type;

    // Construct without allocator
    template <class... _Args>
    __uses_allocator_construction_imp(false_type __use_alloc, _Args&&...__args)
        : m_value(std::forward<_Args>(__args)...) { }

    // Uses-allocator construction, option 1: ignore allocator
    template <class... _Args, class _Alloc>
    __uses_allocator_construction_imp(allocator_arg_t, const _Alloc&,
                                      false_type     __use_alloc,
                                      dont_care_type __prefix_alloc,
                                      dont_care_type __suffix_alloc,
                                      _Args&&...     __args)
        : m_value(std::forward<_Args>(__args)...) { }

    // Uses-allocator construction, option 2: prepend allocator_arg and
    // allocator argument to constructor argument list.
    template <class... _Args, class _Alloc>
    __uses_allocator_construction_imp(allocator_arg_t, const _Alloc& __a,
                                      true_type      __use_alloc,
                                      true_type      __prefix_alloc,
                                      dont_care_type __suffix_alloc,
                                      _Args&&...     __args)
        : m_value(allocator_arg, __a, std::forward<_Args>(__args)...) { }

    // Uses-allocator construction, option 3: append allocator argument to
    // argument list.
    template <class... _Args, class _Alloc>
    __uses_allocator_construction_imp(allocator_arg_t, const _Alloc& __a,
                                      true_type      __use_alloc,
                                      false_type     __prefix_alloc,
                                      true_type      __suffix_alloc,
                                      _Args&&...     __args)
        : m_value(std::forward<_Args>(__args)..., __a) { }

    // Uses-allocator construction, option 4: ill-formed.  An attempt to use
    // it will result a static assertion error.
    template <class... _Args, class _Alloc>
    __uses_allocator_construction_imp(allocator_arg_t, const _Alloc& __a,
                                      true_type      __use_alloc,
                                      false_type     __prefix_alloc,
                                      false_type     __suffix_alloc,
                                      _Args&&...     __args)
        : m_value(std::forward<_Args>(__args)...)
    {
        // Use of sizeof(_Alloc) delays evaluation of static_assert until
        // template instantiation (which should never happen).
        static_assert(sizeof(_Alloc) == 0,
                      "Type uses an allocator but allocator-aware constructor "
                      "is missing");
    }

    _Tp      & value()       { return m_value; }
    _Tp const& value() const { return m_value; }
};

} // end namespace __details

template <class _Tp>
class uses_allocator_construction_wrapper :
    public __details::__uses_allocator_construction_imp<_Tp>
{
    // Wrap an object of type '_Tp' such that we can construct it uniformly
    // with uses-allocator construction (section
    // [allocator.uses.construction]).

    // g++ 4.6.3 doesn't have delegating constuctors, so we must delegate to a
    // base class
    typedef __details::__uses_allocator_construction_imp<_Tp> _Base;

public:
    template <class _Alloc, class... _Args>
    uses_allocator_construction_wrapper(allocator_arg_t,
                                        _Alloc&&   __a,
                                        _Args&&... __args)
        // A delegating constructor would be better here, but g++ 4.6.3
        // doesn't have them.
        : _Base(allocator_arg, __a,
                uses_allocator<_Tp, _Alloc>(),
                is_constructible<_Tp, allocator_arg_t, _Alloc, _Args...>(),
                is_constructible<_Tp, _Args..., _Alloc>(),
                std::forward<_Args>(__args)...) { }

    template <class... _Args>
    uses_allocator_construction_wrapper(_Args&&... __args)
        : _Base(false_type(), std::forward<_Args>(__args)...) { }
};

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_USES_ALLOCATOR_WRAPPER_DOT_H)
