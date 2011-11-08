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
        timestamp,     // Buffer is a datetime.
        interval_ym,   // Buffer is an interval_ym.
        interval_ds,   // Buffer is an interval_ds.
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

    //
    // These specialized auto_descriptor classes allows for transparent
    // transferal of descriptors between auto_descriptor instances. This
    // simplifies the implementation of a private copy of the shared image
    // associated with queries.
    //

    class LIBODB_ORACLE_EXPORT lob_auto_descriptor:
      public auto_descriptor<OCILobLocator>
    {
      typedef auto_descriptor <OCILobLocator> base;

    public:
      lob_auto_descriptor (OCILobLocator* l = 0)
        : base (l)
      {
      }

      lob_auto_descriptor (lob_auto_descriptor& x)
        : base (x.d_)
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

    class LIBODB_ORACLE_EXPORT datetime_auto_descriptor:
      public auto_descriptor<OCIDateTime, dt_timestamp>
    {
      typedef auto_descriptor<OCIDateTime, dt_timestamp> base;

    public:
      datetime_auto_descriptor (OCIDateTime* d = 0):
        base (d)
      {
      }

      datetime_auto_descriptor (datetime_auto_descriptor& x):
        base (x.d_)
      {
        x.d_ = 0;
      }

      datetime_auto_descriptor&
      operator= (datetime_auto_descriptor& x)
      {
        OCIDateTime* l (x.d_);
        x.d_ = 0;
        reset (l);

        return *this;
      }
    };

    class LIBODB_ORACLE_EXPORT interval_ym_auto_descriptor:
      public auto_descriptor<OCIInterval, dt_interval_ym>
    {
      typedef auto_descriptor<OCIInterval, dt_interval_ym> base;

    public:
      interval_ym_auto_descriptor (OCIInterval* d = 0):
        base (d)
      {
      }

      interval_ym_auto_descriptor (interval_ym_auto_descriptor& x):
        base (x.d_)
      {
        x.d_ = 0;
      }

      interval_ym_auto_descriptor&
      operator= (interval_ym_auto_descriptor& x)
      {
        OCIInterval* l (x.d_);
        x.d_ = 0;
        reset (l);

        return *this;
      }
    };

    class LIBODB_ORACLE_EXPORT interval_ds_auto_descriptor:
      public auto_descriptor<OCIInterval, dt_interval_ds>
    {
      typedef auto_descriptor<OCIInterval, dt_interval_ds> base;

    public:
      interval_ds_auto_descriptor (OCIInterval* d = 0):
        base (d)
      {
      }

      interval_ds_auto_descriptor (interval_ds_auto_descriptor& x):
        base (x.d_)
      {
        x.d_ = 0;
      }

      interval_ds_auto_descriptor&
      operator= (interval_ds_auto_descriptor& x)
      {
        OCIInterval* l (x.d_);
        x.d_ = 0;
        reset (l);

        return *this;
      }
    };

    //
    // The OCIDateTime and OCIInterval APIs require that an environment and
    // error handle be passed to any function that manipulates an OCIDateTime
    // or OCIInterval descriptor. It is however impossible to obtain these
    // handles at the time a temporal data image is first initialized. The
    // following structures allow ODB generated code to interact with the OCI
    // temporal descriptor types indirectly via C++ primitives. The wrapped OCI
    // descriptor is then set using these primitives at a time when the
    // required data is available. A symmetric get interface is provided for
    // consistency.
    //

    struct LIBODB_ORACLE_EXPORT datetime
    {
      void
      get (sb2& year,
           ub1& month,
           ub1& day,
           ub1& hour,
           ub1& minute,
           ub1& second,
           ub4& nanosecond) const;

      void
      set (sb2 year,
           ub1 month,
           ub1 day,
           ub1 hour,
           ub1 minute,
           ub1 second,
           ub4 nanosecond);

      void
      set ();

    public:
      OCIEnv* environment;
      OCIError* error;

      datetime_auto_descriptor descriptor;

    private:
      sb2 year_;
      ub1 month_;
      ub1 day_;
      ub1 hour_;
      ub1 minute_;
      ub1 second_;
      ub4 nanosecond_;
    };

    struct LIBODB_ORACLE_EXPORT interval_ym
    {
      void
      get (sb4& year, sb4& month) const;

      void
      set (sb4 year, sb4 month);

      void
      set ();

    public:
      OCIEnv* environment;
      OCIError* error;

      interval_ym_auto_descriptor descriptor;

    private:
      sb4 year_;
      sb4 month_;
    };

    struct LIBODB_ORACLE_EXPORT interval_ds
    {
      void
      get (sb4& day,
           sb4& hour,
           sb4& minute,
           sb4& second,
           sb4& nanosecond) const;

      void
      set (sb4 day,
           sb4 hour,
           sb4 minute,
           sb4 second,
           sb4 nanosecond);

      void
      set ();

    public:
      OCIEnv* environment;
      OCIError* error;

      interval_ds_auto_descriptor descriptor;

    private:
      sb4 day_;
      sb4 hour_;
      sb4 minute_;
      sb4 second_;
      sb4 nanosecond_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_ORACLE_TYPES_HXX
