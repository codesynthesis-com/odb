// file      : odb/mssql/error.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_ERROR_HXX
#define ODB_MSSQL_ERROR_HXX

#include <odb/pre.hxx>
#include <cstddef> // std::size_t

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>     // connection, multiple_exceptions
#include <odb/mssql/auto-handle.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    // Translate ODBC error given a handle and throw (or return, in case
    // multiple_exceptions is not NULL) an appropriate exception.
    //
    LIBODB_MSSQL_EXPORT void
    translate_error (SQLRETURN, connection&, bool end_tran = false);

    LIBODB_MSSQL_EXPORT void
    translate_error (SQLRETURN,
                     connection&,
                     const auto_handle<SQL_HANDLE_STMT>&,
                     std::size_t pos = 0, multiple_exceptions* = 0);

    LIBODB_MSSQL_EXPORT void
    translate_error (SQLRETURN, SQLHANDLE, SQLSMALLINT htype);
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_ERROR_HXX
