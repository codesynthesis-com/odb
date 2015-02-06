// file      : odb/pgsql/pgsql-types.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_PGSQL_TYPES_HXX
#define ODB_PGSQL_PGSQL_TYPES_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/pgsql/pgsql-fwd.hxx> // Oid

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
        boolean_, // Buffer is a bool; size, capacity, truncated are unused.
        smallint, // Buffer is short; size, capacity, truncated are unused.
        integer,  // Buffer is int; size, capacity, truncated are unused.
        bigint,   // Buffer is long long; size, capacity, truncated are unused.
        real,     // Buffer is float; size, capacity, truncated are unused.
        double_,  // Buffer is double; size, capacity, truncated are unused.
        numeric,  // Buffer is a char array.
        date,     // Buffer is int; size, capacity, truncated are unused.
        time,     // Buffer is long long; size, capacity, truncated are unused.
        timestamp,// Buffer is long long; size, capacity, truncated are unused.
        text,     // Buffer is a char array.
        bytea,    // Buffer is a char array.
        bit,      // Buffer is a char array.
        varbit,   // Buffer is a char array.
        uuid      // Buffer is a 16-byte char array; size capacity, truncated
                  // are unused.
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
