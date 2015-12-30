/* pointer_traits.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_POINTER_TRAITS_DOT_H
#define INCLUDED_POINTER_TRAITS_DOT_H

#include "xstd.h"
#include "memory_util.h"

#include <cstddef>

BEGIN_NAMESPACE_XSTD

#ifdef TEMPLATE_ALIASES
# define _REBIND(_U) rebind<_U>
#else
# define _REBIND(_U) rebind<_U>::other
#endif

namespace __details {
    template <typename _Tp> struct __first_param;

    template <template <typename, typename...> class _Tmplt, typename _Tp,
              typename... _Args>
    struct __first_param<_Tmplt<_Tp, _Args...> > {
        typedef _Tp type;
    };

    template <typename _Ptr, typename _Tp>
    struct __ptr_has_rebind {

        template <typename _X>
        static char test(int, typename _X::template _REBIND(_Tp)*);

        template <typename _X>
        static int test(_LowPriorityConversion<int>, void*);

        static const bool value = (1 == sizeof(test<_Ptr>(0, 0)));
    };

    // Implementation of pointer_traits<_Ptr>::rebind if _Ptr has
    // its own rebind template.
    template <typename _Ptr, typename _U,
              bool _HasRebind = __ptr_has_rebind<_Ptr,_U>::value >
    struct __ptr_rebinder
    {
        typedef typename _Ptr::template _REBIND(_U) other;
    };

    // Specialization of pointer_traits<_Ptr>::rebind if _Ptr does not
    // have its own rebind template but has a the form _Ptr<class T,
    // OtherArgs>, where OtherArgs comprises zero or more type parameters.
    // Many pointers fit this form, hence many pointers will get a
    // reasonable default for rebind.
    template <template <class, class...> class _Ptr, typename _Tp, class... _Tn,
              class _U>
    struct __ptr_rebinder<_Ptr<_Tp, _Tn...>,_U,false>
    {
        typedef _Ptr<_U, _Tn...> other;
    };

    _DEFAULT_TYPE_TMPLT(element_type);
    _DEFAULT_TYPE_TMPLT(difference_type);

    template <typename _U, typename _PtrU, typename _PtrT> inline
    auto static_ptr_cast_imp(const _PtrT& p, int) noexcept ->
        decltype(p.template static_pointer_cast<_U>())
    {
        return p.template static_pointer_cast<_U>();
    }

    template <typename _U, typename _PtrU, typename _PtrT> inline
    auto static_ptr_cast_imp(const _PtrT& p,
                             _LowPriorityConversion<int>) noexcept ->
        decltype(static_cast<_PtrU>(p))
    {
        return static_cast<_PtrU>(p);
    }
    
} // end namespace __details

template <typename _Ptr>
struct pointer_traits
{
    typedef _Ptr                                               pointer;
    typedef _DEFAULT_TYPE(_Ptr,element_type,
                          typename __details::__first_param<_Ptr>::type)
                                                               element_type;
    typedef _DEFAULT_TYPE(_Ptr,difference_type,std::ptrdiff_t) difference_type;

#ifdef TEMPLATE_ALIASES
    template <class _U> using rebind =
        typename __details::__ptr_rebinder<_Ptr,_U>::other;
#else
    template <class _U> struct rebind {
        typedef typename __details::__ptr_rebinder<_Ptr,_U>::other other;
    };
#endif

    static
    pointer pointer_to(typename __details::__unvoid<element_type>::type& r)
        { return _Ptr::pointer_to(r); }

    template <typename _U>
    static typename _REBIND(_U) static_pointer_cast(const _Ptr& p) noexcept {
        (void) sizeof(static_cast<_U*>(declval<element_type*>()));
        typedef typename _REBIND(_U) _PtrU;
        return __details::static_ptr_cast_imp<_U, _PtrU>(p, 0);
    }

    template <typename _U>
    static typename _REBIND(_U) const_pointer_cast(const _Ptr& p) noexcept {
        (void) sizeof(const_cast<_U*>(declval<element_type*>()));
        // typedef typename _REBIND(_U) _PtrU;
        // return __details::const_ptr_cast_imp<_U, _PtrU>(p, 0);
        return p.template const_pointer_cast<_U>();
    }

    template <typename _U>
    static typename _REBIND(_U) dynamic_pointer_cast(const _Ptr& p) noexcept {
        return p.template dynamic_pointer_cast<_U>();
    }
};

// Remove cv qualification from _Ptr parameter to pointer_traits:
template <typename _Ptr>
struct pointer_traits<const _Ptr> : pointer_traits<_Ptr> { };
template <typename _Ptr>
struct pointer_traits<volatile _Ptr> : pointer_traits<_Ptr> { };
template <typename _Ptr>
struct pointer_traits<const volatile _Ptr> : pointer_traits<_Ptr> { };

// Remove reference from _Ptr parameter to pointer_traits:
// template <typename _Ptr>
// struct pointer_traits<_Ptr&> : pointer_traits<_Ptr> { };

template <typename _Tp>
struct pointer_traits<_Tp*>
{
    typedef _Tp            element_type;
    typedef _Tp*           pointer;
    typedef std::ptrdiff_t difference_type;

#ifdef TEMPLATE_ALIASES
    template <class _U> using rebind = _U*;
#else
    template <class _U> struct rebind {
        typedef _U* other;
    };
#endif

    static
    pointer pointer_to(typename __details::__unvoid<element_type>::type& r)
        { return xstd::addressof(r); }

    template <typename _U>
    static _U* static_pointer_cast(_Tp* p) noexcept
        { return static_cast<_U*>(p); }
    template <typename _U>
    static _U* const_pointer_cast(_Tp* p) noexcept
        { return const_cast<_U*>(p); }
    template <typename _U>
    static _U* dynamic_pointer_cast(_Tp* p) noexcept
        { return dynamic_cast<_U*>(p); }
};

//#define _PT(_Ptr) pointer_traits<typename std::remove_cv<typename std::remove_reference<_Ptr>::type>::type>
//#define _PT(_Ptr) pointer_traits<typename std::remove_reference<_Ptr>::type>
//#define _PT(_Ptr) pointer_traits<_Ptr>
#define _PT(_Ptr) pointer_traits<typename std::decay<_Ptr>::type>

template <typename _U, typename _Ptr> inline
auto static_pointer_cast(_Ptr&& p) noexcept ->
    typename _PT(_Ptr)::template _REBIND(_U) 
{
    return _PT(_Ptr)::template static_pointer_cast<_U>(std::forward<_Ptr>(p));
}

template <typename _U, typename _Ptr> inline
auto const_pointer_cast(_Ptr&& p) noexcept ->
    typename _PT(_Ptr)::template _REBIND(_U) 
{
    return _PT(_Ptr)::template const_pointer_cast<_U>(std::forward<_Ptr>(p));
}

template <typename _U, typename _Ptr> inline
auto dynamic_pointer_cast(_Ptr&& p) noexcept ->
    typename _PT(_Ptr)::template _REBIND(_U) 
{
    return _PT(_Ptr)::template dynamic_pointer_cast<_U>(std::forward<_Ptr>(p));
}

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_POINTER_TRAITS_DOT_H)
