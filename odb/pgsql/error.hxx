// file      : odb/pgsql/errors.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_ERRORS_HXX
#define ODB_PGSQL_ERRORS_HXX

#include <odb/pre.hxx>

#include <libpq-fe.h>

#include <odb/pgsql/version.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class connection;

    LIBODB_PGSQL_EXPORT void
    translate_connection_error (connection&);

    // Return true if the PGresult is in an error state. If both s and r are
    // non-null, the pointed to value will be populated with the result status.
    // Otherwise, s is ignored.
    //
    LIBODB_PGSQL_EXPORT bool
    is_good_result (PGresult* r, ExecStatusType* s = 0);

    // Translate an error condition involving a PGresult*. If r is null, it is
    // assumed that the error was caused due to a bad connection or a memory
    // allocation error. All remaining parameters except c are ignored.  If r
    // is non-null, the error is translated using the provided result status,
    // s. Error descriptions are obtained using r. If clear_result is true,
    // the resources associated with r are released via PQclear().
    //
    LIBODB_PGSQL_EXPORT void
    translate_result_error (connection& c,
                            PGresult* r = 0,
                            ExecStatusType s = PGRES_EMPTY_QUERY,
                            bool clear_result = true);
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_ERRORS_HXX
