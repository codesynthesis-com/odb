// file      : odb/oracle/error.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ERROR_HXX
#define ODB_ORACLE_ERROR_HXX

#include <odb/pre.hxx>

#include <oci.h>

#include <odb/oracle/forward.hxx>
#include <odb/oracle/version.hxx>
#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    void
    translate_error (void* h, ub4 t, sword s, connection*);

    // Translate OCI error given an error handle and throw an appropriate
    // exception.
    //
    inline LIBODB_ORACLE_EXPORT void
    translate_error (OCIError* h, sword s, connection* c = 0)
    {
      translate_error (h, OCI_HTYPE_ERROR, s, c);
    }

    // Translate an OCI error given an environment handle error and throw
    // an appropriate exception.
    //
    inline LIBODB_ORACLE_EXPORT void
    translate_error (OCIEnv* h)
    {
      translate_error (h, OCI_HTYPE_ENV, OCI_ERROR, 0);
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ERROR_HXX
