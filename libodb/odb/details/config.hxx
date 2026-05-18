// file      : odb/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_DETAILS_CONFIG_HXX
#define ODB_DETAILS_CONFIG_HXX

// no pre

// Once we drop support for C++98, we can probably get rid of config.h except
// for the autotools case by fixing ODB_THREADS_CXX11 (and perhaps supporting
// the ODB_THREADS_NONE case via a "global" (command line) define).
//
#ifdef ODB_COMPILER
#  define ODB_THREADS_NONE
#  define LIBODB_STATIC
#elif defined(_MSC_VER)
#  include <odb/details/config-vc.h>
#else
#  include <odb/details/config.h>
#endif

// no post

#endif // ODB_DETAILS_CONFIG_HXX
