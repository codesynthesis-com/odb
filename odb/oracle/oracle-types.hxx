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
    struct bind
    {
      ub2 type;       // The type stored by buffer. This must be an external
                      // OCI type identifier of the form SQLT_XXX.
      void* buffer;   // Data buffer pointer.
      ub2* size;      // The number of bytes in buffer. When parameter
                      // callbacks are in use, this is interpreted as a ub4*
                      // indicating the current position. When 'type'
                      // specifies a LOB type, this is interpreted as an
                      // OCILobLocator*.
      ub4 capacity;   // The maximum number of bytes that can be stored in
                      // buffer.
      sb2* indicator; // Pointer to an OCI indicator variable.

      enum piece
      {
        whole,
        first,
        next,
        last
      };

      // Callback function signature used to specify LOB parameters that are
      // passed to the database.
      //
      typedef bool (*param_callback_type) (
        void* context,     // [in/out] The user context.
        void** buffer,     // [out] On return, a pointer to a buffer containing
                           // parameter data.
        ub4* size,         // [out] The parameter data length in bytes.
        ub4* position,     // [in/out] A position context. This value remains
                           // unchanged between callback invocations.
        piece*,            // [out] The piece type for this segment of data.
        void* temp_buffer, // [in] A temporary buffer that may be used if
                           // required. The 'buffer' parameter should specify
                           // this buffer on return if it is used.
        ub4 capacity);     // [in] The temporary buffer length in bytes.

      // Callback function signature used to specify LOB values returned from
      // the database.
      //
      typedef bool (*result_callback_type) (
        void* context, // [in/out] The user context.
        void* buffer,  // [in] A buffer containing the result data.
        ub4 size,      // [in] The result data length in bytes.
        piece);        // [in] The piece type for this segment of data.

      union
      {
        param_callback_type param;
        result_callback_type result;
      } callback;

      // This pointer is provided to the user through the 'context' parameter
      // in both parameter and result callback functions.
      //
      void* callback_context;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ORACLE_TYPES_HXX
