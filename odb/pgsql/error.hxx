// file      : odb/pgsql/error.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_ERROR_HXX
#define ODB_PGSQL_ERROR_HXX

#include <odb/pre.hxx>

#include <libpq-fe.h>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx> // connection
#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    // Translate an error condition involving a PGresult*. If r is null, it is
    // assumed that the error was caused due to a bad connection or a memory
    // allocation error.
    //
    LIBODB_PGSQL_EXPORT void
    translate_error (connection& c, PGresult* r);

    // Return true if the PGresult is in an error state. If both s and r are
    // non-null, the pointed to value will be populated with the result status.
    // Otherwise, s is ignored.
    //
    LIBODB_PGSQL_EXPORT bool
    is_good_result (PGresult* r, ExecStatusType* s = 0);
  }
}

#include <odb/pgsql/error.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_ERROR_HXX
