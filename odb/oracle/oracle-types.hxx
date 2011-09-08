// file      : odb/oracle/oracle-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ORACLE_TYPES_HXX
#define ODB_ORACLE_ORACLE_TYPES_HXX

#include <odb/pre.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-fwd.hxx>

namespace odb
{
  namespace oracle
  {
    struct bind
    {
      ub2 type;          // The type stored by buffer. This must be an
                         // external OCI type identifier of the form SQLT_XXX.
      void* buffer;
      ub2* size;         // The number of bytes in buffer.
      sb4 capacity;      // The maximum number of bytes that can be stored in
                         // buffer.
      sb2* indicator;    // Pointer to an OCI indicator variable.
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ORACLE_TYPES_HXX
