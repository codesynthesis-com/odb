// file      : odb/oracle/version.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifdef LIBODB_ORACLE_BUILD2
#  include <odb/oracle/version-build2.hxx>
#else

#ifndef ODB_ORACLE_VERSION_HXX
#define ODB_ORACLE_VERSION_HXX

#include <odb/pre.hxx>

#include <odb/oracle/details/config.hxx>
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
#if ODB_VERSION != 20474
#  error incompatible odb interface version detected
#endif

// libodb-oracle version: odb interface version plus the bugfix
// version.
//
#define LIBODB_ORACLE_VERSION     2049974
#define LIBODB_ORACLE_VERSION_STR "2.5.0-b.24"

#include <odb/post.hxx>

#endif // ODB_ORACLE_VERSION_HXX
#endif // LIBODB_ORACLE_BUILD2
