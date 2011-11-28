// file      : odb/mssql/mssql-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_MSSQL_TYPES_HXX
#define ODB_MSSQL_MSSQL_TYPES_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>

namespace odb
{
  namespace mssql
  {
    enum chunk_type
    {
      first_chunk,
      next_chunk,
      last_chunk,
      one_chunk,
      null_chunk
    };

    typedef void (*param_callback_type) (
      const void* context,   // User context.
      std::size_t* position, // Position context. Am implementation is free
                             // to use this to track position information. It
                             // is initialized to zero before the first call.
      const void** buffer,   // [out] Buffer contaning the data.
      std::size_t* size,     // [out] Data size.
      chunk_type*,           // [out] The position of this chunk of data.
      void* temp_buffer,     // A temporary buffer that may be used by the
                             // implementation.
      std::size_t capacity); // Capacity of the temporary buffer.

    typedef void (*result_callback_type) (
      void* context,         // User context.
      std::size_t* position, // Position context. Am implementation is free
                             // to use this to track position information. It
                             // is initialized to zero before the first call.
      void** buffer,         // [out] Buffer to copy the data to.
      std::size_t* size,     // [in/out] In: amount of data copied into the
                             // buffer after the previous call. Out: capacity
                             // of the buffer.
      chunk_type,            // The position of this chunk; first_chunk means
                             // this is the first call, last_chunk means there
                             // is no more data, null_chunk means this value is
                             // NULL, and one_chunk means the value is empty
                             // (in this case *size is 0).
      std::size_t size_left, // Contains the amount of data left or 0 if this
                             // information is not available.
      void* temp_buffer,     // A temporary buffer that may be used by the
                             // implementation.
      std::size_t capacity); // Capacity of the temporary buffer.

    struct long_callback
    {
      union
      {
        param_callback_type param;
        result_callback_type result;
      } callback;

      union
      {
        const void* param;
        void* result;
      } context;
    };

    struct bind
    {
      // This enumeration identifies the possible buffer types that can be
      // bound to a parameter or result. In most cases, these map directly
      // to SQL_XXX/SQL_C_XXX codes.
      //
      enum buffer_type
      {
        bit,            // Buffer is a 1-byte integer.
        tinyint,        // Buffer is a 1-byte integer.
        smallint,       // Buffer is a 2-byte integer.
        int_,           // Buffer is a 4-byte integer.
        bigint,         // Buffer is an 8-byte integer.

        /*
        numeric,        // Buffer is an SQL_NUMERIC_STRUCT.

        smallmoney,     // Buffer is a 4-byte integer (*10,000 value).
        money,          // Buffer is an 8-byte integer (*10,000 value).
        */

        float4,         // Buffer is a float.
        float8,         // Buffer is a double.

        string,         // Buffer is a char array.
        long_string,    // Buffer is a long_callback.

        nstring,        // Buffer is a wchar_t (2-byte) array.
        long_nstring,   // Buffer is a long_callback.

        binary,         // Buffer is a byte array.
        long_binary,    // Buffer is a long_callback.

        /*
        date,           // Buffer is an SQL_DATE_STRUCT.
        time,           // Buffer is an SQL_SS_TIME2_STRUCT.
        datetime,       // Buffer is an SQL_TIMESTAMP_STRUCT.
        datetimeoffset, // Buffer is an SQL_SS_TIMESTAMPOFFSET_STRUCT.

        uuid,           // Buffer is a 16-byte array (or SQLGUID).
        rowversion,     // Buffer is an 8-byte array.
        */

        last            // Used as an end of list marker.
      };

      buffer_type type; // The buffer type.
      void* buffer;     // The buffer. For long data this is a long_callback.
      SQLLEN* size_ind; // Pointer to the size/inidicator variable.
      SQLLEN capacity;  // Buffer capacity. For string/binary parameters
                        // this value is also used as maximum column size.
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_MSSQL_TYPES_HXX
