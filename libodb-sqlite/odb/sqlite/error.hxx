// file      : odb/sqlite/error.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_ERROR_HXX
#define ODB_SQLITE_ERROR_HXX

#include <odb/pre.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    class connection;

    // Translate SQLite error and throw an appropriate exception.
    //
    LIBODB_SQLITE_EXPORT void
    translate_error (int error, connection&);
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_ERROR_HXX
