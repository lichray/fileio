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
#include "pointer_traits.h"

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

template <typename _Alloc, typename _Tp>
struct __alloc_has_rebind {

    template <typename _X>
    static char test(int, typename _X::template rebind<_Tp>::other*);

    template <typename _X>
    static int test(_LowPriorityConversion<int>, void*);

    static const bool value = (1 == sizeof(test<_Alloc>(0, 0)));
};

// Implementation of allocator_traits<_Alloc>::rebind if _Alloc has
// its own rebind template.
template <typename _Alloc, typename _U,
          bool _HasRebind = __alloc_has_rebind<_Alloc,_U>::value >
struct __alloc_rebinder
{
    typedef typename _Alloc::template rebind<_U>::other other;
};

// Specialization of allocator_traits<_Alloc>::rebind if _Alloc does not
// have its own rebind template but has a the form _Alloc<class T, OtherArgs>,
// where OtherArgs comprises zero or more type parameters.  Most allocators
// fit this form, hence most allocators will get a reasonable default for
// rebind.
template <template <class, class...> class _Alloc, typename _Tp, class... _Tn,
          class _U>
struct __alloc_rebinder<_Alloc<_Tp, _Tn...>,_U,false>
{
    typedef _Alloc<_U, _Tn...> other;
};

_DEFAULT_TYPE_TMPLT(pointer);
_DEFAULT_TYPE_TMPLT(const_pointer);
_DEFAULT_TYPE_TMPLT(void_pointer);
_DEFAULT_TYPE_TMPLT(const_void_pointer);
//_DEFAULT_TYPE_TMPLT(difference_type); // Defined in pointer_traits
_DEFAULT_TYPE_TMPLT(size_type);
_DEFAULT_TYPE_TMPLT(propagate_on_container_copy_assignment);
_DEFAULT_TYPE_TMPLT(propagate_on_container_move_assignment);
_DEFAULT_TYPE_TMPLT(propagate_on_container_swap);

_DEFAULT_FUNC_TMPLT(allocate,return,{
        return v.allocate(a1);
    })

template <typename T, typename... A> inline void __call_ctor(T* p, A&&... a)
    { new ((void*) p) T(std::forward<A>(a)...); }
_DEFAULT_FUNC_TMPLT(construct,,{
        __call_ctor(a1, std::forward<Args>(args)...);
    })

template <typename T> inline void __call_dtor(T* p) { p->~T(); }
_DEFAULT_FUNC_TMPLT(destroy,,{ __call_dtor(a1); })

_DEFAULT_FUNC_TMPLT(max_size,return,{ return ~_Ret(0); })

_DEFAULT_FUNC_TMPLT(select_on_container_copy_construction,return,{
        return v;
    })

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

template <typename Alloc>
struct allocator_traits
{
    typedef Alloc allocator_type;

    typedef typename Alloc::value_type         value_type;

    typedef _DEFAULT_TYPE(Alloc,pointer,value_type*)             pointer;
    typedef _DEFAULT_TYPE(Alloc,const_pointer, typename pointer_traits<pointer>::template rebind<const value_type>::other) const_pointer;
    typedef _DEFAULT_TYPE(Alloc,void_pointer,typename pointer_traits<pointer>::template rebind<void>::other)              void_pointer;
    typedef _DEFAULT_TYPE(Alloc,const_void_pointer,typename pointer_traits<pointer>::template rebind<const void>::other)  const_void_pointer;

    typedef _DEFAULT_TYPE(Alloc,difference_type,std::ptrdiff_t) difference_type;
    typedef _DEFAULT_TYPE(Alloc,size_type,std::size_t)          size_type;

#ifdef TEMPLATE_ALIASES  // template aliases not supported in g++-4.4.1
    template <typename T> using rebind_alloc =
        __details::__alloc_rebinder<Alloc,T>::other;
    template <typename T> using rebind_traits =
        allocator_traits<rebind_alloc<T> >;
#else // !TEMPLATE_ALIASES
    template <typename T>
    struct rebind_alloc : __details::__alloc_rebinder<Alloc,T>::other
    {
        typedef typename __details::__alloc_rebinder<Alloc,T>::other _Base;

        template <typename... Args>
	rebind_alloc(Args&&... args) : _Base(std::forward<Args>(args)...) { }
    };

    template <typename T>
    struct rebind_traits :
        allocator_traits<typename __details::__alloc_rebinder<Alloc,T>::other>
    {
    };
#endif // !TEMPLATE_ALIASES

    static pointer allocate(Alloc& a, size_type n)
        { return a.allocate(n); }
    static pointer allocate(Alloc& a, size_type n, const_void_pointer hint)
        { return _DEFAULT_FUNC(allocate,pointer)(a, n, hint); }

    static void deallocate(Alloc& a, pointer p, size_type n)
        { a.deallocate(p, n); }

    template <typename T, typename... Args>
    static void construct(Alloc& a, T* p, Args&&... args) {
        _DEFAULT_FUNC(construct,void)(a, p, std::forward<Args>(args)...);
    }

    template <typename T>
    static void destroy(Alloc& a, T* p) {
        _DEFAULT_FUNC(destroy,void)(a, p);
    }

    static size_type max_size(const Alloc& a)
        { return _DEFAULT_FUNC(max_size,size_type)(a, 0); }

    // Allocator propagation on construction
    static Alloc select_on_container_copy_construction(const Alloc& rhs) {
        return _DEFAULT_FUNC(select_on_container_copy_construction,Alloc)(rhs);
    }

    // Allocator propagation on assignment and swap.
    // Return true if lhs is modified.
    typedef _DEFAULT_TYPE(Alloc,propagate_on_container_copy_assignment,std::false_type) propagate_on_container_copy_assignment;
    typedef _DEFAULT_TYPE(Alloc,propagate_on_container_move_assignment,std::false_type) propagate_on_container_move_assignment;
    typedef _DEFAULT_TYPE(Alloc,propagate_on_container_swap,std::false_type) propagate_on_container_swap;

};

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_ALLOCATOR_TRAITS_DOT_H)
