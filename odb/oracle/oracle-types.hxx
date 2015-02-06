// file      : odb/oracle/oracle-types.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_ORACLE_TYPES_HXX
#define ODB_ORACLE_ORACLE_TYPES_HXX

#include <odb/pre.hxx>

#include <odb/details/buffer.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx> // binding
#include <odb/oracle/oracle-fwd.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    enum chunk_position
    {
      chunk_one,
      chunk_first,
      chunk_next,
      chunk_last
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

    struct lob_callback
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
      void* buffer;     // Data buffer pointer. For LOB type bindings, this is
                        // interpreted as an oracle::lob*.
      ub2* size;        // The number of bytes in buffer.
      ub4 capacity;     // The maximum number of bytes that can be stored in
                        // the buffer. For LOBs, it used to store array skip
                        // size.
      sb2* indicator;   // Pointer to an OCI indicator variable.

      lob_callback* callback;
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

    // The lob structure wraps data required for both parameter and result
    // LOB type bindings.
    //
    struct LIBODB_ORACLE_EXPORT lob
    {
      ~lob ();
      lob (): locator (0), buffer (0), position (0) {}

      lob (const lob&);
      lob& operator= (const lob&);

    private:
      void
      clone (const lob&);

    public:
      OCIEnv* environment;
      OCIError* error;
      OCILobLocator* locator;

      details::buffer* buffer;
      ub4 position;
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

    // Descriptor management flags.
    //
    const unsigned short descriptor_cache = 0x01;
    const unsigned short descriptor_free  = 0x02;

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

      ~datetime ();
      datetime (unsigned short f = descriptor_cache | descriptor_free)
          : descriptor (0), flags (f) {}

      datetime (const datetime&);
      datetime& operator= (const datetime&);

      // Use the get() and set() functions above unless you know what you
      // are doing and understand how copying of datetime works.
      //
    public:
      OCIEnv* environment;
      OCIError* error;
      OCIDateTime* descriptor;

      unsigned short flags;

    public:
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

      ~interval_ym ();
      interval_ym (unsigned short f = descriptor_cache | descriptor_free)
          : descriptor (0), flags (f) {}

      interval_ym (const interval_ym&);
      interval_ym& operator= (const interval_ym&);

      // Use the get() and set() functions above unless you know what you
      // are doing and understand how copying of interval_ym works.
      //
    public:
      OCIEnv* environment;
      OCIError* error;
      OCIInterval* descriptor;

      unsigned short flags;

    public:
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

      ~interval_ds ();
      interval_ds (unsigned short f = descriptor_cache | descriptor_free)
          : descriptor (0), flags (f) {}

      interval_ds (const interval_ds&);
      interval_ds& operator= (const interval_ds&);

      // Use the get() and set() functions above unless you know what you
      // are doing and understand how copying of interval_ds works.
      //
    public:
      OCIEnv* environment;
      OCIError* error;
      OCIInterval* descriptor;

      unsigned short flags;

    public:
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
