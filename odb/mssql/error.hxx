// file      : odb/mssql/error.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MSSQL_ERROR_HXX
#define ODB_MSSQL_ERROR_HXX

#include <odb/pre.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>     // connection
#include <odb/mssql/auto-handle.hxx>
#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    LIBODB_MSSQL_EXPORT void
    translate_error (const auto_handle<SQL_HANDLE_ENV>&);

    LIBODB_MSSQL_EXPORT void
    translate_error (connection&);
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_ERROR_HXX
