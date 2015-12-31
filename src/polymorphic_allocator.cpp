/* polymorphic_allocator.cpp                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <fileio/polymorphic_allocator.h>

BEGIN_NAMESPACE_XSTD

atomic<polyalloc::memory_resource *>
polyalloc::memory_resource::s_default_resource(nullptr);

polyalloc::new_delete_resource *polyalloc::new_delete_resource_singleton()
    noexcept
{
    // TBD: I think the standard makes this exception-safe, otherwise, we need
    // to use 'call_once()' in '<mutex>'.
    static new_delete_resource singleton;
    return &singleton;
}

END_NAMESPACE_XSTD

// end polymorphic_allocator.cpp
