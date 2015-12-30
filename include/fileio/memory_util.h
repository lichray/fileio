/* memory_util.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include "xstd.h"

#include <cstddef>
#include <memory>

#ifndef INCLUDED_MEMORY_UTIL_DOT_H
#define INCLUDED_MEMORY_UTIL_DOT_H

BEGIN_NAMESPACE_XSTD

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 406 // g++ before 4.6
template <typename _Tp>
inline _Tp* addressof(_Tp& __obj) {
    return static_cast<_Tp*>(
	static_cast<void*>(
	    const_cast<char*>(&reinterpret_cast<const char&>(__obj))));
}
#else
using std::addressof;
#endif

template <typename _Tp>
typename std::add_rvalue_reference<_Tp>::type declval() noexcept;

//static struct allocator_arg_t { } const allocator_arg = { };
using std::allocator_arg_t;
using std::allocator_arg;

namespace __details
{

template <typename _Tp> struct __unvoid { typedef _Tp type; };
template <> struct __unvoid<void> { struct type { }; };
template <> struct __unvoid<const void> { struct type { }; };

template <typename _Tp>
struct _LowPriorityConversion
{
    // Convertible from _Tp with user-defined-conversion rank.
    _LowPriorityConversion(const _Tp&) { }
};

// Infrastructure for providing a default type for _Tp::tname if absent.
#define _DEFAULT_TYPE_TMPLT(tname)                                            \
    template <typename _Tp, typename _Default>                                \
    struct __default_type_ ## tname {                                         \
                                                                              \
        template <typename _X>                                                \
        static char test(int, typename _X::tname*);                           \
                                                                              \
        template <typename _X>                                                \
        static int test(_LowPriorityConversion<int>, void*);                  \
                                                                              \
        struct _DefaultWrap { typedef _Default tname; };                      \
                                                                              \
        static const bool value = (1 == sizeof(test<_Tp>(0, 0)));             \
                                                                              \
        typedef typename                                                      \
            std::conditional<value,_Tp,_DefaultWrap>::type::tname type;       \
    }

#define _DEFAULT_TYPE(T,tname,...)                              \
    typename __details::__default_type_ ## tname<T,__VA_ARGS__>::type

// Infrastructure for providing a default function for _Tp::tname if absent.
struct __void_type { };

#define _DEFAULT_FUNC_TMPLT(fname,doret,default_imp)                          \
    template <typename _Tp, typename... A>                                    \
    auto __default_func_has_ ## fname(int, _Tp& v, A&&... args) ->            \
        decltype((v.fname(std::forward<A>(args)...), std::true_type()));      \
                                                                              \
    template <typename _Tp, typename... A>                                    \
    auto __default_func_has_ ## fname(_LowPriorityConversion<int>,            \
                                      _Tp& v, A&&... args) ->                 \
        std::false_type;                                                      \
                                                                              \
    template <typename _Ret, typename _Tp, typename... Args>                  \
    static _Ret __default_func_dispatch_ ## fname(std::true_type,             \
                                                  _Tp& v, Args&&... args) {   \
        doret v.fname(std::forward<Args>(args)...);                           \
    }                                                                         \
                                                                              \
    template <typename _Ret, typename _Tp, typename A1, typename... Args>     \
    static _Ret __default_func_dispatch_ ## fname(std::false_type,            \
                                                  _Tp& v, A1&& a1,            \
                                                  Args&&... args) {           \
        default_imp                                                           \
    }                                                                         \
                                                                              \
    template <typename _Ret, typename _Tp>                                    \
    static _Ret __default_func_dispatch_ ## fname(std::false_type, _Tp& v){   \
        doret __default_func_dispatch_ ## fname<_Ret>(std::false_type(), v,   \
                                                      __void_type());         \
    }                                                                         \
                                                                              \
    template <typename _Ret, typename _Tp, typename... Args>                  \
    static _Ret __default_func_call_ ## fname(_Tp& v, Args&&... args) {       \
        decltype(__default_func_has_ ## fname(                                \
                     0, v, std::forward<Args>(args)...)) __flag;              \
        doret __default_func_dispatch_ ## fname<_Ret>(__flag, v,              \
                                                 std::forward<Args>(args)...);\
    }

#define _DEFAULT_FUNC(fname,R) __details::__default_func_call_ ## fname<R>

} // end namespace __details

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_MEMORY_UTIL_DOT_H)
