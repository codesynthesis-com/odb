// file      : odb/pgsql/pgsql-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_PGSQL_TYPES_HXX
#define ODB_PGSQL_PGSQL_TYPES_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

namespace odb
{
  namespace pgsql
  {
    // The libpq result binding. This data structures is
    // modelled after MYSQL_BIND from MySQL.
    //
    struct bind
    {
      enum buffer_type
      {
        smallint, // Buffer is short; size, capacity, truncated are unused.
        integer,  // Buffer is int; size, capacity, truncated are unused.
        bigint,   // Buffer is long long; size, capacity, truncated are unused.
        real,     // Buffer is float; size, capacity, truncated are unused.
        double_,  // Buffer is double; size, capacity, truncated are unused.
        numeric,  // Buffer is a char array.
        text,     // Buffer is a char array.
        bytea     // Buffer is a char array.
      };

      buffer_type type;
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
