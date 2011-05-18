// file      : odb/pgsql/pgsql-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_PGSQL_TYPES_HXX
#define ODB_PGSQL_PGSQL_TYPES_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <libpq-fe.h> // Oid

#include <odb/pgsql/pgsql-oid.hxx>

namespace odb
{
  namespace pgsql
  {
    // The libpq result binding. This data structures is
    // modelled after MYSQL_BIND from MySQL.
    //
    struct bind
    {
      Oid type;
      void* buffer;
      std::size_t* size;
      std::size_t capacity;
      bool* is_null;
      bool* truncated;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_PGSQL_TYPES_HXX
