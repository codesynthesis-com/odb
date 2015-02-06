// file      : odb/oracle/error.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ERROR_HXX
#define ODB_ORACLE_ERROR_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/oracle/oracle-fwd.hxx>
#include <odb/oracle/forward.hxx>    // connection, multiple_exceptions
#include <odb/oracle/version.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    // Translate OCI error given an error handle and throw (or return,
    // in case multiple_exceptions is not NULL) an appropriate exception.
    //
    LIBODB_ORACLE_EXPORT void
    translate_error (OCIError*, sword result, connection* = 0,
                     std::size_t pos = 0, multiple_exceptions* = 0);

    LIBODB_ORACLE_EXPORT void
    translate_error (connection&, sword result);

    // Translate an OCI error given an environment handle error and throw
    // an appropriate exception.
    //
    LIBODB_ORACLE_EXPORT void
    translate_error (OCIEnv*);
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ERROR_HXX
