// file      : odb/mssql/mssql-types.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
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
      chunk_null,
      chunk_one,
      chunk_first,
      chunk_next,
      chunk_last,
    };

    typedef void (*param_callback_type) (
      const void* context,   // User context.
      std::size_t* position, // Position context. An implementation is free
                             // to use this to track position information. It
                             // is initialized to zero before the first call.
      const void** buffer,   // [in/out] Buffer contaning the data. On the
                             // the first call it contains a pointer to the
                             // long_callback struct (used for redirections).
      std::size_t* size,     // [out] Data size.
      chunk_type*,           // [out] The position of this chunk of data.
      void* temp_buffer,     // A temporary buffer that may be used by the
                             // implementation.
      std::size_t capacity); // Capacity of the temporary buffer.

    typedef void (*result_callback_type) (
      void* context,         // User context.
      std::size_t* position, // Position context. An implementation is free
                             // to use this to track position information. It
                             // is initialized to zero before the first call.
      void** buffer,         // [in/out] Buffer to copy the data to. On the
                             // the first call it contains a pointer to the
                             // long_callback struct (used for redirections).
      std::size_t* size,     // [in/out] In: amount of data copied into the
                             // buffer after the previous call. Out: capacity
                             // of the buffer.
      chunk_type,            // The position of this chunk; chunk_first means
                             // this is the first call, chunk_last means there
                             // is no more data, chunk_null means this value is
                             // NULL, and chunk_one means the value is empty.
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
        bit,              // Buffer is a 1-byte integer.
        tinyint,          // Buffer is a 1-byte integer.
        smallint,         // Buffer is a 2-byte integer.
        int_,             // Buffer is a 4-byte integer.
        bigint,           // Buffer is an 8-byte integer.

        decimal,          // Buffer is a decimal struct (SQL_NUMERIC_STRUCT).

        smallmoney,       // Buffer is a smallmoney struct (DBMONEY4).
        money,            // Buffer is a money struct (DBMONEY).

        float4,           // Buffer is a float.
        float8,           // Buffer is a double.

        string,           // Buffer is a char array.
        long_string,      // Buffer is a long_callback.

        nstring,          // Buffer is a ucs2_char array.
        long_nstring,     // Buffer is a long_callback.

        binary,           // Buffer is a byte array.
        long_binary,      // Buffer is a long_callback.

        date,             // Buffer is a date struct (SQL_DATE_STRUCT).
        time,             // Buffer is a time struct (SQL_SS_TIME2_STRUCT).
        datetime,         // Buffer is a datetime struct
                          // (SQL_TIMESTAMP_STRUCT).
        datetimeoffset,   // Buffer is a datetimeoffset
                          // (SQL_SS_TIMESTAMPOFFSET_STRUCT).

        uniqueidentifier, // Buffer is a uniqueidentifier struct (SQLGUID).
        rowversion,       // Buffer is an 8-byte array.

        last              // Used as an end of list marker.
      };

      buffer_type type; // The buffer type.
      void* buffer;     // The buffer. For long data this is a long_callback.
      SQLLEN* size_ind; // Pointer to the size/inidicator variable. Size is
                        // ignored except for variable-size, [small]money, and
                        // rowversion types. Sepcial indicator values are
                        // SQL_NULL_DATA (value is NULL) and SQL_DATA_AT_EXEC
                        // (should be set for the long_* data types).
      SQLLEN capacity;  // Buffer capacity. Only used for variable-size
                        // types as well as to pass column/size precisions
                        // as follows: For string/binary parameters this
                        // value (minus one character for strings) is used
                        // as maximum column size. For decimal parameters
                        // it contains precision (p) and scale (s) encoded
                        // as (p * 100 + s). For float4 and float8 it
                        // contains precision. For time, datetime, and
                        // datatimeoffset it contains fractional seconds
                        // (scale). In case of datetime, the special
                        // value 8 indicates the SMALLDATETIME type
                        // which has no seconds.
    };

    // An instance of this structure specifies the function to invoke and
    // the context to pass when the object/view image is about to be
    // modified. This mechanism is used by the query machinery to save the
    // image between result iteration and dereferencing if something gets
    // executed between these two operations that would overwrite the
    // image.
    //
    struct change_callback
    {
      change_callback (): callback (0), context (0) {};

      void (*callback) (void*);
      void* context;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_MSSQL_TYPES_HXX
