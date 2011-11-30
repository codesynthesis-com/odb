// file      : odb/oracle/error.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ERROR_HXX
#define ODB_ORACLE_ERROR_HXX

#include <odb/pre.hxx>

#include <odb/oracle/oracle-fwd.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/version.hxx>
#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    // Translate OCI error given an error handle and throw an appropriate
    // exception.
    //
    LIBODB_ORACLE_EXPORT void
    translate_error (OCIError* h, sword result);

    LIBODB_ORACLE_EXPORT void
    translate_error (connection& c, sword result);

    // Translate an OCI error given an environment handle error and throw
    // an appropriate exception.
    //
    LIBODB_ORACLE_EXPORT void
    translate_error (OCIEnv* h);
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ERROR_HXX
