// file      : odb/details/unused.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_UNUSED_DETAILS_HXX
#define ODB_UNUSED_DETAILS_HXX

#include <odb/pre.hxx>

// VC++ and xlC don't like the (void)x expression if x is a reference
// to an incomplete type. On the other hand, GCC warns that (void*)&x
// doesn't have any effect.
//
#if defined(_MSC_VER) || defined(__xlC__)
#  define ODB_POTENTIALLY_UNUSED(x) (void*)&x
#else
#  define ODB_POTENTIALLY_UNUSED(x) (void)x
#endif

#include <odb/post.hxx>

#endif // ODB_UNUSED_DETAILS_HXX
