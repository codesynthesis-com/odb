// file      : odb/boost/details/config.hxx
// copyright : Copyright (c) 2005-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_BOOST_DETAILS_CONFIG_HXX
#define ODB_BOOST_DETAILS_CONFIG_HXX

// no pre

#ifdef _MSC_VER
#elif defined(ODB_COMPILER)
#  define LIBODB_BOOST_STATIC_LIB
#else
#  include <odb/boost/details/config.h>
#endif

// no post

#endif // ODB_BOOST_DETAILS_CONFIG_HXX
