// file      : odb/oracle/error.hxx
// copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
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
    translate_error (OCIError*, sword result);

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
