// file      : odb/pgsql/pgsql-types.hxx
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
    // The libpq result binding. This data structures is roughly modeled
    // after MYSQL_BIND from MySQL.
    //
    // Types that may need to grow are bound as pointers to pointers to char
    // array (normally in details::buffer) in order to allow simple offsetting
    // in bulk operation support. Note that if we were to do the same for
    // capacity, we could get rid of the buffer growth tracking altogether.
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
        numeric,  // Buffer is a pointer to pointer to char array.
        date,     // Buffer is int; size, capacity, truncated are unused.
        time,     // Buffer is long long; size, capacity, truncated are unused.
        timestamp,// Buffer is long long; size, capacity, truncated are unused.
        text,     // Buffer is a pointer to pointer to char array.
        bytea,    // Buffer is a pointer to pointer to char array.
        bit,      // Buffer is a pointer to char array.
        varbit,   // Buffer is a pointer to pointer to char array.
        uuid      // Buffer is a 16-byte char array; size capacity, truncated
                  // are unused. Note: big-endian, in RFC 4122/4.1.2 order.
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
