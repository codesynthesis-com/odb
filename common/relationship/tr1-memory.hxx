// file      : common/relationship/tr1-memory.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TR1_MEMORY_HXX
#define TR1_MEMORY_HXX

// Try to include TR1 <memory> in a compiler-specific manner. Define
// HAVE_TR1_MEMORY if successfull. If the compiler does not provide
// native TR1 support, fall-back on the Boost TR1 implementation if
// HAVE_BOOST_TR1 is defined.
//

#include <cstddef> // __GLIBCXX__, _HAS_TR1

// GNU C++ or Intel C++ using libstd++.
//
#if defined (__GNUC__) && __GNUC__ >= 4 && defined (__GLIBCXX__)
#  include <tr1/memory>
#  define HAVE_TR1_MEMORY 1
//
// IBM XL C++.
//
#elif defined (__xlC__) && __xlC__ >= 0x0900
#  define __IBMCPP_TR1__
#  include <memory>
#  define HAVE_TR1_MEMORY 1
//
// VC++ or Intel C++ using VC++ standard library.
//
#elif defined (_MSC_VER) && \
  (_MSC_VER == 1500 && defined (_HAS_TR1) || _MSC_VER > 1500)
#  include <memory>
#  define HAVE_TR1_MEMORY 1
//
// Boost fall-back.
//
#elif defined (HAVE_BOOST_TR1)
#  include <boost/tr1/memory.hpp>
#  define HAVE_TR1_MEMORY 1
#endif

#endif // TR1_MEMORY_HXX
