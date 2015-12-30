/* uses_allocator_wrapper.t.cpp                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <fileio/uses_allocator_wrapper.h>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//
//

//==========================================================================
//                  ASSERT TEST MACRO
//--------------------------------------------------------------------------
static int testStatus = 0;

static void aSsErT(int c, const char *s, int i) {
    if (c) {
        std::cout << __FILE__ << ":" << i << ": error: " << s
                  << "    (failed)" << std::endl;
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

enum { VERBOSE_ARG_NUM = 2, VERY_VERBOSE_ARG_NUM, VERY_VERY_VERBOSE_ARG_NUM };

//=============================================================================
//                  CLASSES FOR TESTING USAGE EXAMPLES
//-----------------------------------------------------------------------------

// To avoid circular dependencies, we cannot use polymorphic_allocator.h, but
// all we really need is a stub type for memory_resource.
BEGIN_NAMESPACE_XSTD
namespace polyalloc {
    struct memory_resource
    {
    };

    memory_resource *get_default_resource()
    {
        static memory_resource x;
        return &x;
    }
}
END_NAMESPACE_XSTD

class UsesNoAlloc
{
    int   m_value;

public:
    UsesNoAlloc(int v = 0) : m_value(v) { }

    int value() const { return m_value; }
};

template <typename Alloc>
class UsesAllocPrefix
{
    Alloc m_alloc;
    int   m_value;

public:
    typedef Alloc allocator_type;

    UsesAllocPrefix(int v = 0) : m_value(v) { }
    UsesAllocPrefix(XSTD::allocator_arg_t, const Alloc& a, int v = 0)
        : m_alloc(a), m_value(v) { }

    Alloc get_allocator() const { return m_alloc; }
    int   value() const { return m_value; }
};

template <typename Alloc>
class UsesAllocSuffix
{
    Alloc m_alloc;
    int   m_value;

public:
    typedef Alloc allocator_type;

    UsesAllocSuffix(const Alloc& a = Alloc())
        : m_alloc(a), m_value(0) { }
    UsesAllocSuffix(int v, const Alloc& a = Alloc())
        : m_alloc(a), m_value(v) { }

    Alloc get_allocator() const { return m_alloc; }
    int   value() const { return m_value; }
};

template <typename Alloc>
class UsesAllocBroken
{
    Alloc m_alloc;
    int   m_value;

public:
    typedef Alloc allocator_type;

    // Constructor doesn't take an allocator argument
    UsesAllocBroken(int v = 0)
        : m_alloc(), m_value(v) { }

    Alloc get_allocator() const { return m_alloc; }
    int   value() const { return m_value; }
};

class UsesTypeErasedAlloc
{
    int m_alloc_id;
    int m_value;

public:
    typedef XSTD::erased_type allocator_type;

    UsesTypeErasedAlloc(int v = 0) : m_alloc_id(0), m_value(v) { }

    template <class Alloc>
    UsesTypeErasedAlloc(XSTD::allocator_arg_t, const Alloc& a, int v = 0)
        : m_alloc_id(a.id()), m_value(v) { }

    int get_allocator_id() const { return m_alloc_id; }
    int value() const { return m_value; }
};

class UsesResourcePrefix
{
    typedef XSTD::polyalloc::memory_resource memory_resource;

    memory_resource *m_alloc;
    int                 m_value;

public:
    typedef memory_resource *allocator_type;

    UsesResourcePrefix(int v = 0)
        : m_alloc(XSTD::polyalloc::get_default_resource())
        , m_value(v) { }
    UsesResourcePrefix(XSTD::allocator_arg_t, memory_resource *a, int v = 0)
        : m_alloc(a), m_value(v) { }

    memory_resource *get_memory_resource() const { return m_alloc; }
    int value() const { return m_value; }
};

class UsesResourceSuffix
{
    typedef XSTD::polyalloc::memory_resource memory_resource;

    memory_resource *m_alloc;
    int                 m_value;

public:
    typedef memory_resource *allocator_type;

    UsesResourceSuffix(memory_resource *a =
                       XSTD::polyalloc::get_default_resource())
        : m_alloc(a), m_value(0) { }
    UsesResourceSuffix(int v, memory_resource *a =
                       XSTD::polyalloc::get_default_resource())
        : m_alloc(a), m_value(v) { }

    memory_resource *get_memory_resource() const { return m_alloc; }
    int value() const { return m_value; }
};

// Allocator stub.  No allocations are actually done with this allocator, so
// it is not necessary to actually implement the allocation and deallocation
// functions.  What is important is to be able to identify the allocator
// instance.
template <class T>
class StubAllocator
{
    int m_id;  // Identify this instance and copies of this instance.

public:
    typedef T value_type;

    // Declared but not defined
    T* allocate(std::size_t n, void* hint = 0);
    void deallocate(T* p, std::size_t n);

    explicit StubAllocator(int id = 0) : m_id(id) { }

    int id() const { return m_id; }
};

template <class T>
inline
bool operator==(const StubAllocator<T>& a, const StubAllocator<T>& b)
{
    return a.id() == b.id();
}

template <class T>
inline
bool operator!=(const StubAllocator<T>& a, const StubAllocator<T>& b)
{
    return a.id() != b.id();
}

// memory_resource stub. No allocations are actually done with this
// resource, so the allocation and deallocation functions can be omitted.
// What is important is to be able to identify the allocator instance.
struct StubResource : XSTD::polyalloc::memory_resource
{
};

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    typedef XSTD::polyalloc::memory_resource memory_resource;

    int test = argc > 1 ? atoi(argv[1]) : 0;
    // verbose = argc > 2;
    // veryVerbose = argc > 3;
    // veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__;
    if (test != 0)
        std::cout << " CASE " << test << std::endl;
    else
        std::cout << " all cases" << std::endl;

    switch (test) { case 0: // Do all cases for test-case 0
      case 1: {
        // --------------------------------------------------------------------
        // Test uses-allocator construction option 1: Ignore allocator
        //
        // Concerns: Can construct a wrapper such that allocator is discarded
        //     and not passed to wrapped type's constructor argument list.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nuses-allocator construction option 1"
                  << "\n====================================\n";

        typedef UsesNoAlloc Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubAllocator<int>  MyAlloc(1);

        Wrapper w1;
        ASSERT(w1.value().value() == 0);

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);

        Wrapper w3(XSTD::allocator_arg, MyAlloc);
        ASSERT(w3.value().value() == 0);

        Wrapper w4(XSTD::allocator_arg, MyAlloc, 99);
        ASSERT(w4.value().value() == 99);

      } if (test != 0) break;

      case 2: {
        // --------------------------------------------------------------------
        // Test uses-allocator construction option 2: Prefix allocator
        //
        // Concerns: Can construct a wrapper where allocator is automatically
        //     prepended to wrapped type's constructor argument list.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nuses-allocator construction option 2"
                  << "\n====================================\n";

        typedef UsesAllocPrefix<StubAllocator<int> > Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubAllocator<int>  DfltAlloc;
        StubAllocator<int>  MyAlloc(1);

        Wrapper w1;
        ASSERT(w1.value().value() == 0);
        ASSERT(w1.value().get_allocator() == DfltAlloc);

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);
        ASSERT(w2.value().get_allocator() == DfltAlloc);

        Wrapper w3(XSTD::allocator_arg, MyAlloc);
        ASSERT(w3.value().value() == 0);
        ASSERT(w3.value().get_allocator() == MyAlloc);

        Wrapper w4(XSTD::allocator_arg, MyAlloc, 99);
        ASSERT(w4.value().value() == 99);
        ASSERT(w4.value().get_allocator() == MyAlloc);

      } if (test != 0) break;

      case 3: {
        // --------------------------------------------------------------------
        // Test uses-allocator construction option 3: Suffix allocator
        //
        // Concerns: Can construct a wrapper where allocator is automatically
        //     appended to wrapped type's constructor argument list.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nuses-allocator construction option 3"
                  << "\n====================================\n";

        typedef UsesAllocSuffix<StubAllocator<int> > Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubAllocator<int>  DfltAlloc;
        StubAllocator<int>  MyAlloc(1);

        Wrapper w1;
        ASSERT(w1.value().value() == 0);
        ASSERT(w1.value().get_allocator() == DfltAlloc);

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);
        ASSERT(w2.value().get_allocator() == DfltAlloc);

        Wrapper w3(XSTD::allocator_arg, MyAlloc);
        ASSERT(w3.value().value() == 0);
        ASSERT(w3.value().get_allocator() == MyAlloc);

        Wrapper w4(XSTD::allocator_arg, MyAlloc, 99);
        ASSERT(w4.value().value() == 99);
        ASSERT(w4.value().get_allocator() == MyAlloc);

      } if (test != 0) break;

      case 4: {
        // --------------------------------------------------------------------
        // Test type-erased allocator
        //
        // Concerns: Can construct a wrapper where allocator is automatically
        //     appended to wrapped type's constructor argument list.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\ntype-erased allocator"
                  << "\n=====================\n";

        typedef UsesTypeErasedAlloc Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubAllocator<int>  DfltAlloc;
        StubAllocator<int>  MyAlloc(1);

        Wrapper w1;
        ASSERT(w1.value().value() == 0);
        ASSERT(w1.value().get_allocator_id() == DfltAlloc.id());

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);
        ASSERT(w2.value().get_allocator_id() == DfltAlloc.id());

        Wrapper w3(XSTD::allocator_arg, MyAlloc);
        ASSERT(w3.value().value() == 0);
        ASSERT(w3.value().get_allocator_id() == MyAlloc.id());

        Wrapper w4(XSTD::allocator_arg, MyAlloc, 99);
        ASSERT(w4.value().value() == 99);
        ASSERT(w4.value().get_allocator_id() == MyAlloc.id());

      } if (test != 0) break;

      case 5: {
        // --------------------------------------------------------------------
        // Test memory_resource at front of argument list
        //
        // Concerns: Can construct a wrapper where memory_resource pointer
        //     is automatically prepended to wrapped type's constructor
        //     argument list.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nmemory_resource at front of argument list"
                  << "\n============================================\n";

        typedef UsesResourcePrefix Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubResource myRsrc;
        memory_resource *pDfltRsrc = XSTD::polyalloc::get_default_resource();
        memory_resource *pMyRsrc   = &myRsrc;

        Wrapper w1;
        ASSERT(w1.value().value() == 0);
        ASSERT(w1.value().get_memory_resource() == pDfltRsrc);

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);
        ASSERT(w2.value().get_memory_resource() == pDfltRsrc);

        Wrapper w3(XSTD::allocator_arg, pMyRsrc);
        ASSERT(w3.value().value() == 0);
        ASSERT(w3.value().get_memory_resource() == pMyRsrc);

        Wrapper w4(XSTD::allocator_arg, pMyRsrc, 99);
        ASSERT(w4.value().value() == 99);
        ASSERT(w4.value().get_memory_resource() == pMyRsrc);

      } if (test != 0) break;

      case 6: {
        // --------------------------------------------------------------------
        // Test memory_resource at end of argument list
        //
        // Concerns: Can construct a wrapper where memory_resource is
        //     automatically appended to wrapped type's constructor argument
        //     list.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nmemory_resource at end of argument list"
                  << "\n=======================================\n";

        typedef UsesResourceSuffix Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubResource myRsrc;
        memory_resource *pDfltRsrc = XSTD::polyalloc::get_default_resource();
        memory_resource *pMyRsrc   = &myRsrc;

        Wrapper w1;
        ASSERT(w1.value().value() == 0);
        ASSERT(w1.value().get_memory_resource() == pDfltRsrc);

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);
        ASSERT(w2.value().get_memory_resource() == pDfltRsrc);

        Wrapper w3(XSTD::allocator_arg, pMyRsrc);
        ASSERT(w3.value().value() == 0);
        ASSERT(w3.value().get_memory_resource() == pMyRsrc);

        Wrapper w4(XSTD::allocator_arg, pMyRsrc, 99);
        ASSERT(w4.value().value() == 99);
        ASSERT(w4.value().get_memory_resource() == pMyRsrc);

      } if (test != 0) break;

#ifdef NEGATIVE_COMPILATION_TEST
      case -1: {
        // --------------------------------------------------------------------
        // Test uses-allocator construction option 4: ill-formed
        //
        // Concerns: NEGATIVE TEST: compilation will fail if object has an
        //     allocator_type typedef but does not support an allocator for
        //     the specified constructor.
        //
        // Plan:
	//
        // Testing:
        //
        // --------------------------------------------------------------------

        std::cout << "\nuses-allocator construction option 4"
                  << "\n====================================\n";

        typedef UsesAllocBroken<StubAllocator<int> > Obj;
        typedef XSTD::uses_allocator_construction_wrapper<Obj> Wrapper;
        StubAllocator<int>  DfltAlloc;
        StubAllocator<int>  MyAlloc(1);

        Wrapper w1;
        ASSERT(w1.value().value() == 0);
        ASSERT(w1.value().get_allocator() == DfltAlloc);

        Wrapper w2(99);
        ASSERT(w2.value().value() == 99);
        ASSERT(w2.value().get_allocator() == DfltAlloc);

        Wrapper w3(XSTD::allocator_arg, MyAlloc);
        ASSERT(w3.value().value() == 0);
        ASSERT(w3.value().get_allocator() == MyAlloc);

        Wrapper w4(XSTD::allocator_arg, MyAlloc, 99);
        ASSERT(w4.value().value() == 99);
        ASSERT(w4.value().get_allocator() == MyAlloc);

      } if (test != 0) break;
#endif // NEGATIVE_COMPILATION_TEST

      // end of test cases
      break;

      default: {
        fprintf(stderr, "WARNING: CASE `%d' NOT FOUND.\n", test);
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        fprintf(stderr, "Error, non-zero test status = %d.\n", testStatus);
    }

    return testStatus;
}
