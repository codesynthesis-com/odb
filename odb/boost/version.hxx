// file      : odb/boost/version.hxx
// copyright : Copyright (c) 2005-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifdef LIBODB_BOOST_BUILD2
#  include <odb/boost/version-build2.hxx>
#else

#ifndef ODB_BOOST_VERSION_HXX
#define ODB_BOOST_VERSION_HXX

#include <odb/pre.hxx>

#include <odb/version.hxx>

// Version format is AABBCCDD where
//
// AA - major version number
// BB - minor version number
// CC - bugfix version number
// DD - alpha / beta (DD + 50) version number
//
// When DD is not 00, 1 is subtracted from AABBCC. For example:
//
// Version     AABBCCDD
// 2.0.0       02000000
// 2.1.0       02010000
// 2.1.1       02010100
// 2.2.0.a1    02019901
// 3.0.0.b2    02999952
//

// Check that we have compatible ODB version.
//
#if ODB_VERSION != 20462
#  error incompatible odb interface version detected
#endif

// ODB Boost interface version: odb interface version plus the Boost interface
// version.
//
#define ODB_BOOST_VERSION     2046200
#define ODB_BOOST_VERSION_STR "2.5.0-b.12"

// libodb-boost version: odb interface version plus the bugfix version. Note
// that LIBODB_BOOST_VERSION is always greater or equal to ODB_BOOST_VERSION
// since if the Boost interface virsion is incremented then the bugfix version
// must be incremented as well.
//
#define LIBODB_BOOST_VERSION     2049962
#define LIBODB_BOOST_VERSION_STR "2.5.0-b.12"

#include <odb/post.hxx>

#endif // ODB_BOOST_VERSION_HXX
#endif // LIBODB_BOOST_BUILD2
