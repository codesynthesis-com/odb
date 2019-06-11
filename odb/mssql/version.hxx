// file      : odb/mssql/version.hxx
// copyright : Copyright (c) 2005-2019 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifdef LIBODB_MSSQL_BUILD2
#  include <odb/mssql/version-build2.hxx>
#else

#ifndef ODB_MSSQL_VERSION_HXX
#define ODB_MSSQL_VERSION_HXX

#include <odb/pre.hxx>

#include <odb/mssql/details/config.hxx>
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
#if ODB_VERSION != 20466
#  error incompatible odb interface version detected
#endif

// libodb-mssql version: odb interface version plus the bugfix
// version.
//
#define LIBODB_MSSQL_VERSION     2049966
#define LIBODB_MSSQL_VERSION_STR "2.5.0-b.16"

#include <odb/post.hxx>

#endif // ODB_MSSQL_VERSION_HXX
#endif // LIBODB_MSSQL_BUILD2
