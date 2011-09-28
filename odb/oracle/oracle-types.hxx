// file      : odb/oracle/oracle-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ORACLE_TYPES_HXX
#define ODB_ORACLE_ORACLE_TYPES_HXX

#include <odb/pre.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-fwd.hxx>

namespace odb
{
  namespace oracle
  {
    enum chunk_position
    {
      one_chunk,
      first_chunk,
      next_chunk,
      last_chunk
    };

    // Callback function signature used to specify LOB parameters that are
    // passed to the database. If false is returned from the callback,
    // statement execution is aborted.
    //
    typedef bool (*param_callback_type) (
      const void* context,   // [in] The user context.
      ub4* position_context, // [in] A position context. A callback is free to
                             // use this to track position information.
      void** buffer,         // [out] On return, a pointer to a buffer
                             // containing parameter data.
      ub4* size,             // [out] The parameter data size in bytes.
      chunk_position*,       // [out] The position of the chunk of data in
                             // buffer.
      void* temp_buffer,     // [in] A temporary buffer that may be used if
                             // required. The buffer argument should specify
                             // this buffer on return if it is used.
      ub4 capacity);         // [in] The temporary buffer size in bytes.

    // Callback function signature used to specify LOB values returned from
    // the database. If false is returned, database_exception is thrown.
    //
    typedef bool (*result_callback_type) (
      void* context,   // [in] The user context.
      void* buffer,    // [in] A buffer containing the result data.
      ub4 size,        // [in] The result data size in bytes.
      chunk_position); // [in] The position of this chunk.

    union lob_callback
    {
      param_callback_type param;
      result_callback_type result;
    };

    struct bind
    {
      // This enumeration identifies the possible buffer types that can be
      // bound to a bind instance. In most cases, these map directly to
      // SQLT_XXX codes, identifying an external OCI type. nstring and nclob
      // however have no equivalent OCI typecode. These additional identifiers
      // allow for a consistent interface across all types. Note that these
      // values are mapped to their corresponding external OCI typecodes (if
      // any) using their integer values, and should therefore not be
      // rearranged or explicitly assigned without also adjusting the
      // sqlt_lookup array in odb/oracle/statement.cxx.
      //
      enum buffer_type
      {
        integer,       // Buffer is an integer type of size specified by size.
        uinteger,      // Buffer is an unsigned integer of size specified by
                       // size.
        binary_float,  // Buffer is a float.
        binary_double, // Buffer is a double.
        number,        // Buffer is a variable length char array.
        date,          // Buffer is a 7-byte char array.
        timestamp,     // Buffer is a variable length char array.
        string,        // Buffer is a variable length char array.
        nstring,       // Buffer is a variable length char array.
        blob,          // Bind is a callback.
        clob,          // Bind is a callback.
        nclob,         // Bind is a callback.
        last           // Used as an end of list marker.
      };

      buffer_type type; // The type stored by buffer. This must be an external
                        // OCI type identifier of the form SQLT_XXX.
      void* buffer;     // Data buffer pointer.
      ub2* size;        // The number of bytes in buffer. When parameter
                        // callbacks are in use, this is interpreted as a ub4*
                        // indicating the current position. When result
                        // callbacks are in use, this is interpreted as an
                        // OCILobLocator*.
      ub4 capacity;     // The maximum number of bytes that can be stored in
                        // buffer.
      sb2* indicator;   // Pointer to an OCI indicator variable.

      lob_callback callback;

      // This pointer is provided to the user through the context argument
      // in both parameter and result callback functions.
      //
      void* callback_context;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ORACLE_TYPES_HXX
