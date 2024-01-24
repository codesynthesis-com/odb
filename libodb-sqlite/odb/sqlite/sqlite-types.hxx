// file      : odb/sqlite/sqlite-types.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_SQLITE_TYPES_HXX
#define ODB_SQLITE_SQLITE_TYPES_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

namespace odb
{
  namespace sqlite
  {
    // The SQLite parameter/result binding. This data structures is modelled
    // after MYSQL_BIND from MySQL.
    //
    struct bind
    {
      enum buffer_type
      {
        integer, // Buffer is long long; size, capacity, truncated are unused.
        real,    // Buffer is double; size, capacity, truncated are unused.
        text,    // Buffer is a UTF-8 char array.
        text16,  // Buffer is a UTF-16 2-byte char array (sizes in bytes).
        blob,    // Buffer is a char array.
        stream   // Buffer is stream_buffers. Size specifies the BLOB size
                 // (input only). Capacity and truncated unused.
      };

      buffer_type type;
      void* buffer;
      std::size_t* size;
      std::size_t capacity;
      bool* is_null;
      bool* truncated;
    };

    // The "out" values should be set in set_image() to point to
    // variables that will be receiving the data. The "in" values
    // are used in set_value() and contain the data that needs to
    // be copied over.
    //
    struct stream_buffers
    {
      union {std::string* out; const char* in;} db;
      union {std::string* out; const char* in;} table;
      union {std::string* out; const char* in;} column;
      union {long long* out; long long in;} rowid;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_SQLITE_TYPES_HXX
