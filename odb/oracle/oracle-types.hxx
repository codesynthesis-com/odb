// file      : odb/oracle/oracle-types.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ORACLE_TYPES_HXX
#define ODB_ORACLE_ORACLE_TYPES_HXX

#include <odb/pre.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/oracle-fwd.hxx>
#include <odb/oracle/auto-descriptor.hxx>

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
                             // use this to track position information. This is
                             // initialized to zero before the callback is
                             // invoked for the first time.
      const void** buffer,   // [out] On return, a pointer to a buffer
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
      void* context,         // [in] The user context.
      ub4* position_context, // [in] A position context. A callback is free to
                             // use this to track position information. This is
                             // initialized to zero before the callback is
                             // invoked for the first time.
      void* buffer,          // [in] A buffer containing the result data.
      ub4 size,              // [in] The result data size in bytes.
      chunk_position);       // [in] The position of this chunk.

    union lob_callback
    {
      param_callback_type param;
      result_callback_type result;
    };

    union lob_context
    {
      const void* param;
      void* result;
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
        raw,           // Buffer is a variable length char array.
        blob,          // Bind is a callback.
        clob,          // Bind is a callback.
        nclob,         // Bind is a callback.
        last           // Used as an end of list marker.
      };

      buffer_type type; // The type stored by buffer.
      void* buffer;     // Data buffer pointer. When result callbacks are in
                        // use, this is interpreted as a lob_auto_descriptor*.
      ub2* size;        // The number of bytes in buffer. When parameter
                        // callbacks are in use, this is interpreted as a ub4*
                        // indicating the current position. For LOB result
                        // bindings, this is interpreted as the OCIDefine
                        // handle associated with the LOB result parameter.
      ub4 capacity;     // The maximum number of bytes that can be stored in
                        // buffer.
      sb2* indicator;   // Pointer to an OCI indicator variable.

      lob_callback* callback;

      // This pointer is provided to the user through the context argument
      // in both parameter and result callback functions.
      //
      lob_context* context;
    };

    // An instance of this structure specifies the function to invoke and
    // the context to pass just prior to the image associated with a query
    // being modified.
    //
    struct change_callback
    {
      change_callback (): callback (0), context (0) {};

      void (*callback) (void*);
      void* context;
    };

    // The LOB specialization of auto_descriptor allows for transparent
    // transferal of LOB descriptors between auto_descriptor instances. This
    // simplifies the implementation of a private copy of the shared image
    // associated with queries.
    //
    class LIBODB_ORACLE_EXPORT lob_auto_descriptor
      : auto_descriptor<OCILobLocator>
    {
    public:
      lob_auto_descriptor (OCILobLocator* l = 0)
        : auto_descriptor<OCILobLocator> (l)
      {
      }

      lob_auto_descriptor (lob_auto_descriptor& x)
        : auto_descriptor<OCILobLocator> (x.d_)
      {
        x.d_ = 0;
      }

      lob_auto_descriptor&
      operator= (lob_auto_descriptor& x)
      {
        OCILobLocator* l (x.d_);
        x.d_ = 0;
        reset (l);

        return *this;
      }
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ORACLE_TYPES_HXX
