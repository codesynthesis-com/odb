// file      : odb/mysql/error.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_ERROR_HXX
#define ODB_MYSQL_ERROR_HXX

#include <odb/pre.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/version.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class connection;

    // Translate MySQL error and throw an appropriate exception. Also,
    // if the error code indicates that the connection is no longer
    // usable, mark it as failed.
    //
    LIBODB_MYSQL_EXPORT void
    translate_error (connection&);

    LIBODB_MYSQL_EXPORT void
    translate_error (connection&, MYSQL_STMT*);
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_ERROR_HXX
