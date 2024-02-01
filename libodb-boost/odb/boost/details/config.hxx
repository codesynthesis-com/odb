// file      : odb/boost/details/config.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DETAILS_CONFIG_HXX
#define ODB_BOOST_DETAILS_CONFIG_HXX

// no pre

#ifdef ODB_COMPILER
#  define LIBODB_BOOST_STATIC_LIB
#elif !defined(LIBODB_BOOST_BUILD2)
#  ifdef _MSC_VER
#    include <odb/boost/details/config-vc.h>
#  else
#    include <odb/boost/details/config.h>
#  endif
#endif

// no post

#endif // ODB_BOOST_DETAILS_CONFIG_HXX
