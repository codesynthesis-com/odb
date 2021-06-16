// file      : odb/pgsql/error.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_ERROR_HXX
#define ODB_PGSQL_ERROR_HXX

#include <odb/pre.hxx>

#include <libpq-fe.h>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx> // connection, multiple_exceptions
#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    // Translate an error condition involving PGresult* and throw (or return,
    // in case multiple_exceptions is not NULL) an appropriate exception. If
    // result is NULL, it is assumed that the error was caused due to a bad
    // connection or a memory allocation error.
    //
    LIBODB_PGSQL_EXPORT void
    translate_error (connection& c, PGresult* r,
                     std::size_t pos = 0, multiple_exceptions* = 0);

    // Return true if PGresult is not NULL and is not in an error state. If
    // both s and r are non-NULL, the pointed to value will be populated with
    // the result status. Otherwise, s is ignored.
    //
    LIBODB_PGSQL_EXPORT bool
    is_good_result (PGresult* r, ExecStatusType* s = 0);
  }
}

#include <odb/pgsql/error.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_ERROR_HXX
