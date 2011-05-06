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
    translate_error (connection&);

    LIBODB_PGSQL_EXPORT void
    translate_error (connection& c, PGresult* r);
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_ERRORS_HXX
