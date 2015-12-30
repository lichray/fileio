/* xstd.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_XSTD_DOT_H
#define INCLUDED_XSTD_DOT_H

#include <utility>
#include <type_traits>

#define XSTD xstd
#define BEGIN_NAMESPACE_XSTD namespace XSTD {
#define END_NAMESPACE_XSTD   }
#define USING_NAMESPACE_XSTD using namespace XSTD
#define USING_FROM_NAMESPACE_XSTD(sym) using XSTD::sym

// Define namesapsce
BEGIN_NAMESPACE_XSTD

END_NAMESPACE_XSTD

#endif // ! defined(INCLUDED_XSTD_DOT_H)
