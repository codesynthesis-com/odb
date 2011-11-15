// file      : odb/oracle/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstring> // std::strlen
#include <cassert>

#include <oci.h>

#include <odb/tracer.hxx>
#include <odb/exceptions.hxx> // object_not_persistent
#include <odb/details/unused.hxx>

#include <odb/oracle/database.hxx>
#include <odb/oracle/statement.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/auto-descriptor.hxx>
#include <odb/oracle/error.hxx>
#include <odb/oracle/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    // Mapping of bind::buffer_type values for parameter buffers to their
    // equivalent external OCI typecode identifiers.
    //
    static const ub4 param_sqlt_lookup[bind::last] =
    {
      SQLT_INT,         // bind::integer
      SQLT_UIN,         // bind::uinteger
      SQLT_BFLOAT,      // bind::binary_float
      SQLT_BDOUBLE,     // bind::binary_double
      SQLT_NUM,         // bind::number
      SQLT_DAT,         // bind::date
      SQLT_TIMESTAMP,   // bind::timestamp
      SQLT_INTERVAL_YM, // bind::interval_ym
      SQLT_INTERVAL_DS, // bind::interval_ds
      SQLT_CHR,         // bind::string
      SQLT_CHR,         // bind::nstring
      SQLT_BIN,         // bind::raw
      SQLT_LBI,         // bind::blob
      SQLT_LNG,         // bind::clob
      SQLT_LNG          // bind::nclob
    };

    // Mapping of bind::buffer_type values for result buffers to their
    // equivalent external OCI typecode identifiers.
    //
    static const ub4 result_sqlt_lookup[bind::last] =
    {
      SQLT_INT,         // bind::integer
      SQLT_UIN,         // bind::uinteger
      SQLT_BFLOAT,      // bind::binary_float
      SQLT_BDOUBLE,     // bind::binary_double
      SQLT_NUM,         // bind::number
      SQLT_DAT,         // bind::date
      SQLT_TIMESTAMP,   // bind::timestamp
      SQLT_INTERVAL_YM, // bind::interval_ym
      SQLT_INTERVAL_DS, // bind::interval_ds
      SQLT_CHR,         // bind::string
      SQLT_CHR,         // bind::nstring
      SQLT_BIN,         // bind::raw
      SQLT_BLOB,        // bind::blob
      SQLT_CLOB,        // bind::clob
      SQLT_CLOB         // bind::nclob
    };

    static sb4
    param_callback_proxy (void* context,
                          OCIBind*,
                          ub4,               // iteration
                          ub4,               // index
                          void** buffer,
                          ub4* size,
                          ub1* piece,
                          void** indicator)
    {
      bind& b (*static_cast<bind*> (context));

      details::buffer* lob_buffer (
        reinterpret_cast<details::buffer*> (b.buffer));

      // Only call the callback if the parameter is not NULL.
      //
      if (*b.indicator != -1)
      {
        chunk_position pos;
        if (!(*b.callback->param) (b.context->param,
                                   reinterpret_cast<ub4*> (b.size),
                                   const_cast<const void**> (buffer),
                                   size,
                                   &pos,
                                   lob_buffer->data (),
                                   lob_buffer->capacity ()))
          return OCI_ERROR;

        switch (pos)
        {
        case one_chunk:
          {
            *piece = OCI_ONE_PIECE;
            break;
          }
        case first_chunk:
          {
            *piece = OCI_FIRST_PIECE;
            break;
          }
        case next_chunk:
          {
            *piece = OCI_NEXT_PIECE;
            break;
          }
        case last_chunk:
          {
            *piece = OCI_LAST_PIECE;
            break;
          }
        }
      }
      else
        *piece = OCI_ONE_PIECE;

      *indicator = b.indicator;

      return OCI_CONTINUE;
    }

    //
    // statement
    //

    statement::
    ~statement ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->deallocate (conn_, *this);
      }

      // Unbind (free) parameter descriptors.
      //
      for (size_t i (0); i < usize_; ++i)
      {
        ub4 t;
        bind& b (*udata_[i].bind);

        switch (b.type)
        {
        case bind::timestamp:
          {
            datetime* dt (static_cast<datetime*> (b.buffer));

            if (dt->flags & descriptor_cache)
              dt->descriptor = 0;

            t = OCI_DTYPE_TIMESTAMP;
            break;
          }
        case bind::interval_ym:
          {
            interval_ym* iym (static_cast<interval_ym*> (b.buffer));

            if (iym->flags & descriptor_cache)
              iym->descriptor = 0;

            t = OCI_DTYPE_INTERVAL_YM;
            break;
          }
        case bind::interval_ds:
          {
            interval_ds* ids (static_cast<interval_ds*> (b.buffer));

            if (ids->flags & descriptor_cache)
              ids->descriptor = 0;

            t = OCI_DTYPE_INTERVAL_DS;
            break;
          }
        default:
          {
            assert (false);
            return;
          }
        }

        OCIDescriptorFree (udata_[i].value, t);
      }

      delete[] udata_;
    }

    statement::
    statement (connection& conn, const string& text)
        : conn_ (conn), udata_ (0), usize_ (0)
    {
      init (text.c_str (), text.size ());
    }

    statement::
    statement (connection& conn, const char* text)
        : conn_ (conn), udata_ (0), usize_ (0)
    {
      init (text, strlen (text));
    }

    void statement::
    init (const char* text, size_t text_size)
    {
      OCIError* err (conn_.error_handle ());
      OCIStmt* handle (0);

      sword r (OCIStmtPrepare2 (conn_.handle (),
                                &handle,
                                err,
                                reinterpret_cast<const OraText*> (text),
                                static_cast<ub4> (text_size),
                                0,
                                0,
                                OCI_NTV_SYNTAX,
                                OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      stmt_.reset (handle, OCI_STRLS_CACHE_DELETE, err);

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->prepare (conn_, *this);
      }
    }

    const char* statement::
    text () const
    {
      OCIError* err (conn_.error_handle ());

      OraText* s (0);
      sword r (OCIAttrGet (stmt_,
                           OCI_HTYPE_STMT,
                           &s,
                           0,
                           OCI_ATTR_STATEMENT,
                           err));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      return reinterpret_cast<char*> (s);
    }

    void statement::
    bind_param (bind* b, size_t n)
    {
      // Figure out how many unbind elements we will need and allocate them.
      //
      {
        size_t un (0);

        for (size_t i (0); i < n; ++i)
        {
          switch (b[i].type)
          {
          case bind::timestamp:
            {
              datetime* dt (static_cast<datetime*> (b[i].buffer));
              if (dt->descriptor == 0 && (dt->flags & descriptor_free) == 0)
                un++;
              break;
            }
          case bind::interval_ym:
            {
              interval_ym* iym (static_cast<interval_ym*> (b[i].buffer));
              if (iym->descriptor == 0 && (iym->flags & descriptor_free) == 0)
                un++;
              break;
            }
          case bind::interval_ds:
            {
              interval_ds* ids (static_cast<interval_ds*> (b[i].buffer));
              if (ids->descriptor == 0 && (ids->flags & descriptor_free) == 0)
                un++;
              break;
            }
          default:
            break;
          }
        }

        if (un != 0)
          udata_ = new unbind[un];
      }

      sword r;
      OCIError* err (conn_.error_handle ());
      OCIEnv* env (conn_.database ().environment ());

      // The parameter position in OCIBindByPos is specified as a 1-based
      // index.
      //
      n++;

      for (ub4 i (1); i < n; ++i, ++b)
      {
        void* value (0);
        bool callback (b->callback != 0);

        switch (b->type)
        {
        case bind::timestamp:
          {
            datetime* dt (static_cast<datetime*> (b->buffer));

            if (dt->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_TIMESTAMP,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              if (dt->flags & descriptor_cache)
              {
                dt->descriptor = static_cast<OCIDateTime*> (d);
                dt->environment = env;
                dt->error = err;
              }

              // If the datetime instance is not responsible for the
              // descriptor, then we have to arrange to have it freed
              // using the unbind machinery.
              //
              if ((dt->flags & descriptor_free) == 0)
              {
                unbind& u (udata_[usize_++]);

                u.bind = b;
                u.value = d;
                value = &u.value;
              }
              else
                value = &dt->descriptor;

              // Initialize the descriptor from the cached data.
              //
              if (b->indicator == 0 || *b->indicator != -1)
                r = OCIDateTimeConstruct (env,
                                          err,
                                          static_cast<OCIDateTime*> (d),
                                          dt->year_,
                                          dt->month_,
                                          dt->day_,
                                          dt->hour_,
                                          dt->minute_,
                                          dt->second_,
                                          dt->nanosecond_,
                                          0,
                                          0);

              if (r != OCI_SUCCESS)
                translate_error (err, r);
            }
            else
              value = &dt->descriptor;

            break;
          }
        case bind::interval_ym:
          {
            interval_ym* iym (static_cast<interval_ym*> (b->buffer));

            if (iym->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_INTERVAL_YM,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              if (iym->flags & descriptor_cache)
              {
                iym->descriptor = static_cast<OCIInterval*> (d);
                iym->environment = env;
                iym->error = err;
              }

              // If the interval_ym instance is not responsible for the
              // descriptor, then we have to arrange to have it freed
              // using the unbind machinery.
              //
              if ((iym->flags & descriptor_free) == 0)
              {
                unbind& u (udata_[usize_++]);

                u.bind = b;
                u.value = d;
                value = &u.value;
              }
              else
                value = &iym->descriptor;

              // Initialize the descriptor from the cached data.
              //
              if (b->indicator == 0 || *b->indicator != -1)
                r = OCIIntervalSetYearMonth (env,
                                             err,
                                             iym->year_,
                                             iym->month_,
                                             static_cast<OCIInterval*> (d));

              if (r != OCI_SUCCESS)
                translate_error (err, r);
            }
            else
              value = &iym->descriptor;

            break;
          }
        case bind::interval_ds:
          {
            interval_ds* ids (static_cast<interval_ds*> (b->buffer));

            if (ids->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_INTERVAL_DS,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              if (ids->flags & descriptor_cache)
              {
                ids->descriptor = static_cast<OCIInterval*> (d);
                ids->environment = env;
                ids->error = err;
              }

              // If the interval_ds instance is not responsible for the
              // descriptor, then we have to arrange to have it freed
              // using the unbind machinery.
              //
              if ((ids->flags & descriptor_free) == 0)
              {
                unbind& u (udata_[usize_++]);

                u.bind = b;
                u.value = d;
                value = &u.value;
              }
              else
                value = &ids->descriptor;

              // Initialize the descriptor from the cached data.
              //
              if (b->indicator == 0 || *b->indicator != -1)
                r = OCIIntervalSetDaySecond (env,
                                             err,
                                             ids->day_,
                                             ids->hour_,
                                             ids->minute_,
                                             ids->second_,
                                             ids->nanosecond_,
                                             static_cast<OCIInterval*> (d));

              if (r != OCI_SUCCESS)
                translate_error (err, r);
            }
            else
              value = &ids->descriptor;

            break;
          }
        case bind::blob:
        case bind::clob:
        case bind::nclob:
          {
            details::buffer& lob_buffer (conn_.lob_buffer ());

            if (lob_buffer.capacity () == 0)
              lob_buffer.capacity (4096);

            // Generally, we should not modify the binding structure or
            // image since that would break the thread-safety guarantee
            // of the query expression. However, in Oracle, LOBs cannot
            // be used in queries so we can make an exception here.
            //
            b->buffer = &lob_buffer;

            // When binding LOB parameters, the capacity must be greater than
            // 4000 and less than the maximum LOB length in bytes. If it is
            // not, OCI returns an error. Other than this, the capacity seems
            // to be irrelevant to OCI bind behaviour for LOB parameters when
            // used with callbacks.
            //
            b->capacity = 4096;

            break;
          }
        default:
          {
#if OCI_MAJOR_VERSION < 11 ||                           \
  (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION < 2)
            // Assert if a 64 bit integer buffer type is provided and the OCI
            // version is unable to implicitly convert the NUMBER binary data
            // to the relevant type.
            //
            assert ((b->type != bind::integer &&
                     b->type != bind::uinteger) || b->capacity <= 4);
#endif
            if (!callback)
              value = b->buffer;

            break;
          }
        }

        OCIBind* h (0);
        r = OCIBindByPos (stmt_,
                          &h,
                          err,
                          i,
                          value,
                          static_cast<sb4> (b->capacity),
                          param_sqlt_lookup[b->type],
                          b->indicator,
                          b->size,
                          0,
                          0,
                          0,
                          callback ? OCI_DATA_AT_EXEC : OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        // Set the character set form for national strings.
        //
        if (b->type == bind::nstring || b->type == bind::nclob)
        {
          ub1 form (SQLCS_NCHAR);

          r = OCIAttrSet (h,
                          OCI_HTYPE_BIND,
                          &form,
                          0,
                          OCI_ATTR_CHARSET_FORM,
                          err);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }

        if (callback)
        {
          r = OCIBindDynamic (h, err, b, &param_callback_proxy, 0, 0);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
      }
    }

    void statement::
    bind_result (bind* b, size_t c, size_t p)
    {
      ODB_POTENTIALLY_UNUSED (p);

      sword r;
      OCIError* err (conn_.error_handle ());
      OCIEnv* env (conn_.database ().environment ());

      for (size_t i (1); i <= c; ++i, ++b)
      {
        void* value (0);
        ub2* size (0);

        switch (b->type)
        {
        case bind::timestamp:
          {
            datetime* dt (static_cast<datetime*> (b->buffer));

            if (dt->descriptor == 0)
            {
              assert ((dt->flags & descriptor_cache) &&
                      (dt->flags & descriptor_free));

              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_TIMESTAMP,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              dt->descriptor = static_cast<OCIDateTime*> (d);
              dt->environment = env;
              dt->error = err;
            }

            value = &dt->descriptor;
            break;
          }
        case bind::interval_ym:
          {
            interval_ym* iym (static_cast<interval_ym*> (b->buffer));

            if (iym->descriptor == 0)
            {
              assert ((iym->flags & descriptor_cache) &&
                      (iym->flags & descriptor_free));

              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_INTERVAL_YM,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              iym->descriptor = static_cast<OCIInterval*> (d);
              iym->environment = env;
              iym->error = err;
            }

            value = &iym->descriptor;
            break;
          }
        case bind::interval_ds:
          {
            interval_ds* ids (static_cast<interval_ds*> (b->buffer));

            if (ids->descriptor == 0)
            {
              assert ((ids->flags & descriptor_cache) &&
                      (ids->flags & descriptor_free));

              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_INTERVAL_DS,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              ids->descriptor = static_cast<OCIInterval*> (d);
              ids->environment = env;
              ids->error = err;
            }

            value = &ids->descriptor;
            break;
          }
        case bind::blob:
        case bind::clob:
        case bind::nclob:
          {
            auto_descriptor<OCILobLocator>* lob (
              static_cast<auto_descriptor<OCILobLocator>*> (b->buffer));

            if (lob->get () == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_LOB, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              lob->reset (static_cast<OCILobLocator*> (d));
            }

            value = &lob->get ();
            break;
          }
        default:
          {
#if OCI_MAJOR_VERSION < 11 || \
  (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION < 2)
            // Assert if a 64 bit integer buffer type is provided and the OCI
            // version is unable to implicitly convert the NUMBER binary data
            // to the relevant type.
            //
            assert ((b->type != bind::integer && b->type != bind::uinteger) ||
                    b->capacity <= 4);
#endif
            value = b->buffer;
            size = b->size;

            break;
          }
        }

        OCIDefine* h (0);
        r = OCIDefineByPos (stmt_,
                            &h,
                            err,
                            i,
                            value,
                            static_cast<sb4> (b->capacity),
                            result_sqlt_lookup[b->type],
                            b->indicator,
                            size,
                            0,
                            OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        if (b->type == bind::blob ||
            b->type == bind::clob ||
            b->type == bind::nclob)
        {
          // The OCIDefine handle is stored in the size member of the bind in
          // case the LOB parameter is rebound. If rebinding is necessary, the
          // same OCIDefine handle is used.
          //
          b->size = reinterpret_cast<ub2*> (h);

          // LOB prefetching is only supported in OCI version 11.1 and greater
          // and in Oracle server 11.1 and greater. If this code is called
          // against a pre 11.1 server, the call to OCIAttrSet will return an
          // error code.
          //
#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >= 1) \
  || OCI_MAJOR_VERSION > 11
          if (p != 0)
          {
            ub4 n (static_cast<ub4> (p));

            r = OCIAttrSet (h,
                            OCI_HTYPE_DEFINE,
                            &n,
                            0,
                            OCI_ATTR_LOBPREFETCH_SIZE,
                            err);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err, r);
          }
#endif
        }
        else if (b->type == bind::nstring)
        {
          ub1 form (SQLCS_NCHAR);

          r = OCIAttrSet (h,
                          OCI_HTYPE_DEFINE,
                          &form,
                          0,
                          OCI_ATTR_CHARSET_FORM,
                          err);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
      }
    }

    void statement::
    rebind_result (bind* b, size_t c, size_t p)
    {
      ODB_POTENTIALLY_UNUSED (p);

      sword r;
      OCIEnv* env (conn_.database ().environment ());

      for (size_t i (1); i <= c; ++i, ++b)
      {
        void* value (0);

        switch (b->type)
        {
        case bind::timestamp:
          {
            datetime* dt (static_cast<datetime*> (b->buffer));

            if (dt->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_TIMESTAMP,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              dt->descriptor = static_cast<OCIDateTime*> (d);
            }

            value = &dt->descriptor;
            break;
          }
        case bind::interval_ym:
          {
            interval_ym* iym (static_cast<interval_ym*> (b->buffer));

            if (iym->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_INTERVAL_YM,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              iym->descriptor = static_cast<OCIInterval*> (d);
            }

            value = &iym->descriptor;
            break;
          }
        case bind::interval_ds:
          {
            interval_ds* ids (static_cast<interval_ds*> (b->buffer));

            if (ids->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env,
                                      &d,
                                      OCI_DTYPE_INTERVAL_DS,
                                      0,
                                      0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              ids->descriptor = static_cast<OCIInterval*> (d);
            }

            value = &ids->descriptor;
            break;
          }
        case bind::blob:
        case bind::clob:
        case bind::nclob:
          {
            auto_descriptor<OCILobLocator>* lob (
              reinterpret_cast<auto_descriptor<OCILobLocator>*> (b->buffer));

            if (lob->get () == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_LOB, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              lob->reset (static_cast <OCILobLocator*> (d));
            }

            value = &lob->get ();
            break;
          }
        default:
          {
            continue;
          }
        }

        // The bind::size member of bind instances associated with LOB and
        // TIMESTAMP type is interpreted as the OCIDefine* returned by the
        // initial call to OCIDefineByPos when binding for the first time.
        //
        OCIDefine* h (reinterpret_cast<OCIDefine*> (b->size));
        OCIError* err (conn_.error_handle ());

        r = OCIDefineByPos (stmt_,
                            &h,
                            err,
                            i,
                            value,
                            static_cast<sb4> (b->capacity),
                            result_sqlt_lookup[b->type],
                            b->indicator,
                            0,
                            0,
                            OCI_DEFINE_SOFT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);


        // LOB prefetching is only supported in OCI version 11.1 and greater
        // and in Oracle server 11.1 and greater. If this code is called
        // against a pre 11.1 server, the call to OCIAttrSet will return an
        // error code.
        //
        // Note that even though we are re-binding the same handle, we still
        // have to reset this attribute. Failing to do so will result in the
        // mysterious ORA-03106 fatal two-task communication protocol error.
        //
#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >= 1) \
  || OCI_MAJOR_VERSION > 11
        if (p != 0 && (b->type == bind::blob ||
                       b->type == bind::clob ||
                       b->type == bind::nclob))
        {
          ub4 n (static_cast<ub4> (p));

          r = OCIAttrSet (h,
                          OCI_HTYPE_DEFINE,
                          &n,
                          0,
                          OCI_ATTR_LOBPREFETCH_SIZE,
                          err);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
#endif
      }
    }

    void statement::
    stream_result (bind* b, size_t c)
    {
      OCIError* err (conn_.error_handle ());

      for (size_t i (0); i < c; ++i, ++b)
      {
        // Only stream if the bind specifies a LOB type, and the LOB value is
        // not NULL, and a result callback has been provided.
        //
        if ((b->type == bind::blob ||
             b->type == bind::clob ||
             b->type == bind::nclob) &&
            (b->indicator == 0 || *b->indicator != -1) &&
            b->callback->result != 0)
        {
          auto_descriptor<OCILobLocator>& locator (
            *reinterpret_cast<auto_descriptor<OCILobLocator>*> (b->buffer));

          ub4 position (0); // Position context.
          ub1 piece (OCI_FIRST_PIECE);

          // Setting the value pointed to by the byte_amt argument to 0 on the
          // first call to OCILobRead2 instructs OCI to remain in a polling
          // state until the EOF is reached, at which point OCILobRead2 will
          // return OCI_SUCCESS.
          //
          ub8 read (0);
          ub1 cs_form (b->type == bind::nclob ? SQLCS_NCHAR : SQLCS_IMPLICIT);

          // Allocate buffer space if necessary.
          //
          details::buffer& lob_buffer (conn_.lob_buffer ());

          if (lob_buffer.capacity () == 0)
            lob_buffer.capacity (4096);

          sword r;
          do
          {
            r = OCILobRead2 (conn_.handle (),
                             err,
                             locator,
                             &read,
                             0,
                             1,
                             lob_buffer.data (),
                             lob_buffer.capacity (),
                             piece,
                             0,
                             0,
                             0,
                             cs_form);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err, r);

            chunk_position cp;

            if (piece == OCI_FIRST_PIECE)
              cp = r == OCI_SUCCESS ? one_chunk : first_chunk;
            else if (r == OCI_NEED_DATA)
              cp = next_chunk;
            else
              cp = last_chunk;

            piece = OCI_NEXT_PIECE;

            // OCI generates and ORA-24343 error when an error code is
            // returned from a user callback. We simulate this.
            //
            if (!(*b->callback->result) (b->context->result,
                                         &position,
                                         lob_buffer.data (),
                                         static_cast<ub4> (read),
                                         cp))
              throw database_exception (24343, "user defined callback error");

          } while (r == OCI_NEED_DATA);
        }
      }
    }

    //
    // generic_statement
    //

    generic_statement::
    generic_statement (connection& conn, const string& text)
        : statement (conn, text), bound_ (false)
    {
      init ();
    }

    generic_statement::
    generic_statement (connection& conn, const char* text)
        : statement (conn, text), bound_ (false)
    {
      init ();
    }

    void generic_statement::
    init ()
    {
      OCIError* err (conn_.error_handle ());

      sword r (OCIAttrGet (stmt_,
                           OCI_HTYPE_STMT,
                           &stmt_type_,
                           0,
                           OCI_ATTR_STMT_TYPE,
                           err));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);
    }

    generic_statement::
    ~generic_statement ()
    {
    }

    unsigned long long generic_statement::
    execute ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      sword r (0);

      OCISvcCtx* handle (conn_.handle ());
      OCIError* err (conn_.error_handle ());

      if (stmt_type_ == OCI_STMT_SELECT)
      {
        // Do not prefetch any rows.
        //
        r = OCIStmtExecute (handle, stmt_, err, 0, 0, 0, 0, OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        // In order to successfully execute a select statement, OCI/Oracle
        // requires that there be OCIDefine handles provided for all select
        // list columns. Since we are not interested in any data returned by
        // the select statement, all buffer pointers, indicator variable
        // pointers, and data length pointers are specified as NULL (we still
        // specify a valid data type identifier; not doing so results in
        // undefined behavior). This results in truncation errors being
        // returned for all attempted row fetches. However, cursor behaves
        // normally allowing us to return the row count for a select
        // statement. Note also that we only need to do this once.
        //
        if (!bound_)
        {
          for (ub4 i (1); ; ++i)
          {
            auto_descriptor<OCIParam> param;
            {
              OCIParam* p (0);
              r = OCIParamGet (stmt_,
                               OCI_HTYPE_STMT,
                               err,
                               reinterpret_cast<void**> (&p),
                               i);

              if (r == OCI_ERROR) // No more result columns.
                break;

              param.reset (p);
            }

            ub2 data_type;
            r = OCIAttrGet (param,
                            OCI_DTYPE_PARAM,
                            &data_type,
                            0,
                            OCI_ATTR_DATA_TYPE,
                            err);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err, r);

            // No need to keep track of the OCIDefine handles - these will
            // be deallocated with the statement.
            //
            OCIDefine* define (0);
            r = OCIDefineByPos (stmt_,
                                &define,
                                err,
                                i,
                                0,    // NULL value buffer pointer
                                0,    // zero length value buffer
                                data_type,
                                0,    // NULL indicator pointer
                                0,    // NULL length data pointer
                                0,    // NULL column level return code pointer
                                OCI_DEFAULT);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err, r);
          }

          bound_ = true;
        }

        for (;;)
        {
          r = OCIStmtFetch2 (stmt_, err, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);

          if (r == OCI_NO_DATA)
            break;
          else if (r == OCI_ERROR)
          {
            sb4 e;
            r = OCIErrorGet (err, 1, 0, &e, 0, 0, OCI_HTYPE_ERROR);

            // ORA-01406 is returned if there is a truncation error. We expect
            // and ignore this error.
            //
            if (e != 1406)
              translate_error (err, r);
          }
          else if (r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
      }
      else
      {
        // OCIStmtExecute requires a non-zero iters param for DML statements.
        //
        r = OCIStmtExecute (handle, stmt_, err, 1, 0, 0, 0, OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);
      }

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      err);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      return row_count;
    }

    //
    // select_statement
    //

    select_statement::
    ~select_statement ()
    {
    }

    select_statement::
    select_statement (connection& conn,
                      const string& text,
                      binding& param,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn, text),
          result_ (result),
          result_version_ (0),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      bind_param (param.bind, param.count);
      bind_result (result.bind, result.count, lob_prefetch_size);
      result_version_ = result_.version;
    }

    select_statement::
    select_statement (connection& conn,
                      const char* text,
                      binding& param,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn, text),
          result_ (result),
          result_version_ (0),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      bind_param (param.bind, param.count);
      bind_result (result.bind, result.count, lob_prefetch_size);
      result_version_ = result_.version;
    }

    select_statement::
    select_statement (connection& conn,
                      const string& text,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn, text),
          result_ (result),
          result_version_ (0),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      bind_result (result.bind, result.count, lob_prefetch_size);
      result_version_ = result_.version;
    }

    select_statement::
    select_statement (connection& conn,
                      const char* text,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn, text),
          result_ (result),
          result_version_ (0),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      bind_result (result.bind, result.count, lob_prefetch_size);
      result_version_ = result_.version;
    }

    void select_statement::
    execute ()
    {
      if (!done_)
        free_result ();

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      OCIError* err (conn_.error_handle ());

      // @@ Retrieve a single row into the already bound output buffers as an
      // optimization? This will avoid multiple server round-trips in the case
      // of a single object load.
      //
      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               err,
                               0,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      done_ = r == OCI_NO_DATA;

#ifndef NDEBUG
      ub4 n (0);
      r = OCIAttrGet (stmt_, OCI_HTYPE_STMT, &n, 0, OCI_ATTR_PARAM_COUNT, err);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      // Make sure that the number of columns in the result returned by
      // the database matches the number that we expect. A common cause
      // of this assertion is a native view with a number of data members
      // not matching the number of columns in the SELECT-list.
      //
      assert (n == result_.count);
#endif
    }

    select_statement::result select_statement::
    fetch ()
    {
      if (!done_)
      {
        change_callback* cc (result_.change_callback);

        if (cc != 0 && cc->callback != 0)
          (cc->callback) (cc->context);

        if (result_version_ != result_.version)
        {
          rebind_result (result_.bind, result_.count, lob_prefetch_size_);
          result_version_ = result_.version;
        }

        sword r (OCIStmtFetch2 (stmt_,
                                conn_.error_handle (),
                                1,
                                OCI_FETCH_NEXT,
                                0,
                                OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (conn_.error_handle (), r);
        else if (r == OCI_NO_DATA)
          done_ = true;
      }

      return done_ ? no_data : success;
    }

    void select_statement::
    free_result ()
    {
      if (!done_)
      {
        sword r (OCIStmtFetch2 (stmt_,
                                conn_.error_handle (),
                                0,
                                OCI_FETCH_NEXT,
                                0,
                                OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (conn_.error_handle (), r);

        done_ = true;
      }
    }

    //
    // insert_statement
    //

    static sb4
    returning_in (void* context,
                  OCIBind*,            // bind
                  ub4,                 // iter
                  ub4,                 // index
                  void** buffer,
                  ub4* size,
                  ub1* piece,
                  void** indicator)
    {
      typedef insert_statement::id_bind_type bind;

      bind& b (*static_cast<bind*> (context));

      *buffer = 0;
      *size = 0;
      *piece = OCI_ONE_PIECE;
      b.indicator = -1;
      *indicator = &b.indicator;

      return OCI_CONTINUE;
    }

    static sb4
    returning_out (void* context,
                   OCIBind*,           // bind
                   ub4,                // iter
                   ub4,                // index
                   void** buffer,
                   ub4** size,
                   ub1*,               // piece
                   void** indicator,
                   ub2** rcode)
    {
      typedef insert_statement::id_bind_type bind;

      bind& b (*static_cast<bind*> (context));

#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >=2) \
  || OCI_MAJOR_VERSION > 11
      *buffer = &b.id.integer;
      **size = sizeof (unsigned long long);
#else
      *buffer = b.id.number.buffer;
      *size = &b.id.number.size;
      b.id.number.size = 21;
#endif

      *indicator = &b.indicator;
      *rcode = 0;

      return OCI_CONTINUE;
    }

    insert_statement::
    ~insert_statement ()
    {
    }

    insert_statement::
    insert_statement (connection& conn,
                      const string& text,
                      binding& param,
                      bool returning)
        : statement (conn, text)
    {
      init (param, returning);
    }

    insert_statement::
    insert_statement (connection& conn,
                      const char* text,
                      binding& param,
                      bool returning)
        : statement (conn, text)
    {
      init (param, returning);
    }

    void insert_statement::
    init (binding& param, bool returning)
    {
      bind_param (param.bind, param.count);

      if (returning)
      {
        OCIError* err (conn_.error_handle ());
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               param.count + 1,
                               0,
#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >=2) \
  || OCI_MAJOR_VERSION > 11
                               sizeof (unsigned long long),
                               SQLT_UIN,
#else
                               21,
                               SQLT_NUM,
#endif
                               0,
                               0,
                               0,
                               0,
                               0,
                               OCI_DATA_AT_EXEC));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        r = OCIBindDynamic (h,
                            err,
                            &id_bind_,
                            &returning_in,
                            &id_bind_,
                            &returning_out);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);
      }
    }

    bool insert_statement::
    execute ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      OCIError* err (conn_.error_handle ());

      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               err,
                               1,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
      {
        sb4 e;
        OCIErrorGet (err, 1, 0, &e, 0, 0, OCI_HTYPE_ERROR);

        // The Oracle error code ORA-00001 indicates unique constraint
        // violation, which covers more than just a duplicate primary key.
        // Unfortunately, there is nothing more precise that we can use.
        //
        if (e == 1)
          return false;
        else
          translate_error (err, r);
      }

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      err);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      // The value of the OCI_ATTR_ROW_COUNT attribute associated with an
      // INSERT statment represents the number of rows inserted.
      //
      return row_count != 0;
    }

    unsigned long long insert_statement::
    id ()
    {
#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >=2) \
  || OCI_MAJOR_VERSION > 11
      return id_bind_.id.integer;
#else
      return details::number_to_uint64 (
        id_bind_.id.number.buffer,
        static_cast <std::size_t> (id_bind_.id.number.size));
#endif
    }

    //
    // update_statement
    //

    update_statement::
    ~update_statement ()
    {
    }

    update_statement::
    update_statement (connection& conn, const string& text, binding& param)
        : statement (conn, text)
    {
      bind_param (param.bind, param.count);
    }

    update_statement::
    update_statement (connection& conn, const char* text, binding& param)
        : statement (conn, text)
    {
      bind_param (param.bind, param.count);
    }

    unsigned long long update_statement::
    execute ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      OCIError* err (conn_.error_handle ());

      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               err,
                               1,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      err);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      // The value of the OCI_ATTR_ROW_COUNT attribute associated with an
      // UPDATE statment represents the number of matching rows found. Zero
      // indicates no match.
      //
      return static_cast<unsigned long long> (row_count);
    }

    //
    // delete_statement
    //

    delete_statement::
    ~delete_statement ()
    {
    }

    delete_statement::
    delete_statement (connection& conn, const string& text, binding& param)
        : statement (conn, text)
    {
      bind_param (param.bind, param.count);
    }

    delete_statement::
    delete_statement (connection& conn, const char* text, binding& param)
        : statement (conn, text)
    {
      bind_param (param.bind, param.count);
    }

    unsigned long long delete_statement::
    execute ()
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      OCIError* err (conn_.error_handle ());

      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               err,
                               1,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      err);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      return static_cast<unsigned long long> (row_count);
    }
  }
}
