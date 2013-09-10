// file      : odb/oracle/statement.cxx
// copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <cstring> // std::strlen
#include <cassert>

#include <odb/tracer.hxx>
#include <odb/exceptions.hxx> // object_not_persistent
#include <odb/details/unused.hxx>

#include <odb/oracle/database.hxx>
#include <odb/oracle/statement.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/auto-descriptor.hxx>
#include <odb/oracle/error.hxx>
#include <odb/oracle/exceptions.hxx>

#include <odb/oracle/details/number.hxx>

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

    extern "C" sb4
    odb_oracle_param_callback_proxy (void* context,
                                     OCIBind*,
                                     ub4,               // iteration
                                     ub4,               // index
                                     void** buffer,
                                     ub4* size,
                                     ub1* piece,
                                     void** indicator)
    {
      bind& b (*static_cast<bind*> (context));
      lob* l (static_cast<lob*> (b.buffer));

      // Only call the callback if the parameter is not NULL.
      //
      if (*b.indicator != -1)
      {
        chunk_position pos;
        if (!(*b.callback->callback.param) (
              b.callback->context.param,
              &l->position,
              const_cast<const void**> (buffer),
              size,
              &pos,
              l->buffer->data (),
              l->buffer->capacity ()))
          return OCI_ERROR;

        switch (pos)
        {
        case chunk_one:
          {
            *piece = OCI_ONE_PIECE;
            break;
          }
        case chunk_first:
          {
            *piece = OCI_FIRST_PIECE;
            break;
          }
        case chunk_next:
          {
            *piece = OCI_NEXT_PIECE;
            break;
          }
        case chunk_last:
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
      if (empty ())
        return;

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
        bind* b (udata_[i].bind);

        switch (udata_[i].type)
        {
        case bind::timestamp:
          {
            if (b != 0)
              static_cast<datetime*> (b->buffer)->descriptor = 0;

            t = OCI_DTYPE_TIMESTAMP;
            break;
          }
        case bind::interval_ym:
          {
            if (b != 0)
              static_cast<interval_ym*> (b->buffer)->descriptor = 0;

            t = OCI_DTYPE_INTERVAL_YM;
            break;
          }
        case bind::interval_ds:
          {
            if (b != 0)
              static_cast<interval_ds*> (b->buffer)->descriptor = 0;

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
    statement (connection_type& conn,
               const string& text,
               statement_kind sk,
               const binding* process,
               bool optimize)
        : conn_ (conn), udata_ (0), usize_ (0)
    {
      init (text.c_str (), text.size (), sk, process, optimize);
    }

    statement::
    statement (connection_type& conn,
               const char* text,
               statement_kind sk,
               const binding* process,
               bool optimize)
        : conn_ (conn), udata_ (0), usize_ (0)
    {
      init (text, strlen (text), sk, process, optimize);
    }

    void statement::
    init (const char* text,
          size_t text_size,
          statement_kind sk,
          const binding* proc,
          bool optimize)
    {
      string tmp;
      if (proc != 0)
      {
        switch (sk)
        {
        case statement_select:
          process_select (text,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          '"', '"',
                          optimize,
                          tmp,
                          false); // No AS in JOINs.
          break;
        case statement_insert:
          process_insert (text,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          ':',
                          tmp);
          break;
        case statement_update:
          process_update (text,
                          &proc->bind->buffer, proc->count, sizeof (bind),
                          ':',
                          tmp);
          break;
        case statement_delete:
        case statement_generic:
          assert (false);
        }

        text = tmp.c_str ();
        text_size = tmp.size ();
      }

      // Empty statement.
      //
      if (*text == '\0')
        return;

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
        translate_error (conn_, r);

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

    ub4 statement::
    bind_param (bind* b, size_t n)
    {
      // Figure out how many unbind elements we will need and allocate them.
      //
      {
        size_t un (0);

        for (size_t i (0); i < n; ++i)
        {
          if (b[i].buffer == 0) // Skip NULL entries.
            continue;

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

      bool seen_lob (false);
      sword r;
      OCIError* err (conn_.error_handle ());
      OCIEnv* env (conn_.database ().environment ());

      ub4 i (0);
      for (bind* end (b + n); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

        i++; // Column index is 1-based.

        void* value;
        sb4 capacity;
        ub2* size (0);
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

                u.type = bind::timestamp;
                u.bind = (dt->flags & descriptor_cache) ? b : 0;
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

            capacity = static_cast<sb4> (sizeof (OCIDateTime*));

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

                u.type = bind::interval_ym;
                u.bind = (iym->flags & descriptor_cache) ? b : 0;
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

            capacity = static_cast<sb4> (sizeof (OCIInterval*));

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

                u.type = bind::interval_ds;
                u.bind = (ids->flags & descriptor_cache) ? b : 0;
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

            capacity = static_cast<sb4> (sizeof (OCIInterval*));

            break;
          }
        case bind::blob:
        case bind::clob:
        case bind::nclob:
          {
            seen_lob = true;

            lob* l (static_cast<lob*> (b->buffer));

            if (l->buffer == 0)
            {
              details::buffer& lob_buffer (conn_.lob_buffer ());

              if (lob_buffer.capacity () == 0)
                lob_buffer.capacity (4096);

              // Generally, we should not modify the image since that would
              // break the thread-safety guarantee of the query expression.
              // However, in Oracle, LOBs cannot be used in queries so we can
              // make an exception here.
              //
              l->buffer = &lob_buffer;
            }

            assert (callback);
            value = 0;

            // When binding LOB parameters, the capacity must be greater than
            // 4000 and less than the maximum LOB length in bytes. If it is
            // not, OCI returns an error. Other than this, the capacity seems
            // to be irrelevant to OCI bind behaviour for LOB parameters when
            // used with callbacks.
            //
            capacity = 4096;

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
            value = callback ? 0 : b->buffer;
            capacity = static_cast<sb4> (b->capacity);
            size = b->size;

            break;
          }
        }

        OCIBind* h (0);
        r = OCIBindByPos (stmt_,
                          &h,
                          err,
                          i,
                          value,
                          capacity,
                          param_sqlt_lookup[b->type],
                          b->indicator,
                          size,
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

        if (seen_lob && (b->type == bind::string || b->type == bind::nstring))
        {
          // Set the maximum data size for all string types. If this is not set
          // Oracle server will implicitly calculate this maximum size. If the
          // calculated size exceeds 4000 bytes (which may occur if a character
          // set conversion is required) and the string is bound after a LOB
          // binding, the server will return an ORA-24816 error.
          //
          sb4 n (4000);
          r = OCIAttrSet (h,
                          OCI_HTYPE_BIND,
                          &n,
                          0,
                          OCI_ATTR_MAXDATA_SIZE,
                          err);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }

        if (callback)
        {
          r = OCIBindDynamic (
            h, err, b, &odb_oracle_param_callback_proxy, 0, 0);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
      }

      return i;
    }

    ub4 statement::
    bind_result (bind* b, size_t c, size_t p)
    {
      ODB_POTENTIALLY_UNUSED (p);

      sword r;
      OCIError* err (conn_.error_handle ());
      OCIEnv* env (conn_.database ().environment ());

      ub4 i (0);
      for (bind* end (b + c); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

        i++; // Column index is 1-based.

        void* value;
        sb4 capacity;
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
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_TIMESTAMP, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              dt->descriptor = static_cast<OCIDateTime*> (d);
              dt->environment = env;
              dt->error = err;
            }

            value = &dt->descriptor;
            capacity = static_cast<sb4> (sizeof (OCIDateTime*));

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
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_INTERVAL_YM, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              iym->descriptor = static_cast<OCIInterval*> (d);
              iym->environment = env;
              iym->error = err;
            }

            value = &iym->descriptor;
            capacity = static_cast<sb4> (sizeof (OCIInterval*));

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
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_INTERVAL_DS, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              ids->descriptor = static_cast<OCIInterval*> (d);
              ids->environment = env;
              ids->error = err;
            }

            value = &ids->descriptor;
            capacity = static_cast<sb4> (sizeof (OCIInterval*));

            break;
          }
        case bind::blob:
        case bind::clob:
        case bind::nclob:
          {
            lob* l (static_cast<lob*> (b->buffer));

            if (l->locator == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_LOB, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              l->locator = static_cast<OCILobLocator*> (d);
            }

            value = &l->locator;
            capacity = static_cast<sb4> (sizeof (OCILobLocator*));

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
            capacity = static_cast<sb4> (b->capacity);
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
                            capacity,
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

      return i;
    }

    void statement::
    rebind_result (bind* b, size_t c, size_t p)
    {
      ODB_POTENTIALLY_UNUSED (p);

      sword r;
      OCIEnv* env (conn_.database ().environment ());

      ub4 i (0);
      for (bind* end (b + c); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

        i++; // Column index is 1-based.

        void* value;

        switch (b->type)
        {
        case bind::timestamp:
          {
            datetime* dt (static_cast<datetime*> (b->buffer));

            if (dt->descriptor == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_TIMESTAMP, 0, 0);

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
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_INTERVAL_YM, 0, 0);

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
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_INTERVAL_DS, 0, 0);

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
            lob* l (static_cast<lob*> (b->buffer));

            if (l->locator == 0)
            {
              void* d (0);
              r = OCIDescriptorAlloc (env, &d, OCI_DTYPE_LOB, 0, 0);

              if (r != OCI_SUCCESS)
                throw invalid_oci_handle ();

              l->locator = static_cast<OCILobLocator*> (d);
            }

            value = &l->locator;
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
                            static_cast<sb4> (sizeof (void*)),
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
    stream_result (bind* b, size_t c, void* obase, void* nbase)
    {
      OCIError* err (conn_.error_handle ());

      for (bind* end (b + c); b != end; ++b)
      {
        if (b->buffer == 0) // Skip NULL entries.
          continue;

        // Only stream if the bind specifies a LOB type.
        //
        if (b->type == bind::blob ||
            b->type == bind::clob ||
            b->type == bind::nclob)
        {
          lob* l;
          sb2* ind;
          lob_callback* cb;

          if (obase == 0)
          {
            l = static_cast<lob*> (b->buffer);
            ind = b->indicator;
            cb = b->callback;
          }
          else
          {
            // Re-base the pointers.
            //
            char* ob (static_cast<char*> (obase));
            char* nb (static_cast<char*> (nbase));

            char* p (static_cast<char*> (b->buffer));
            assert (ob <= p);
            l = reinterpret_cast<lob*> (nb + (p - ob));

            if (b->indicator == 0)
              ind = 0;
            else
            {
              p = reinterpret_cast<char*> (b->indicator);
              assert (ob <= p);
              ind = reinterpret_cast<sb2*> (nb + (p - ob));
            }

            p = reinterpret_cast<char*> (b->callback);
            assert (ob <= p);
            cb = reinterpret_cast<lob_callback*> (nb + (p - ob));
          }

          // Nothing to do if the LOB value is NULL or the result callback
          // hasn't been provided.
          //
          if ((ind != 0 && *ind == -1) || cb->callback.result == 0)
            continue;

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
                             l->locator,
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
              translate_error (conn_, r);

            chunk_position cp;

            if (piece == OCI_FIRST_PIECE)
              cp = r == OCI_SUCCESS ? chunk_one : chunk_first;
            else if (r == OCI_NEED_DATA)
              cp = chunk_next;
            else
              cp = chunk_last;

            piece = OCI_NEXT_PIECE;

            // OCI generates and ORA-24343 error when an error code is
            // returned from a user callback. We simulate this.
            //
            if (!(*cb->callback.result) (
                  cb->context.result,
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
    generic_statement (connection_type& conn, const string& text)
        : statement (conn,
                     text, statement_generic,
                     0, false),
          bound_ (false)
    {
      init ();
    }

    generic_statement::
    generic_statement (connection_type& conn, const char* text)
        : statement (conn,
                     text, statement_generic,
                     0, false),
          bound_ (false)
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
          translate_error (conn_, r);

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
              translate_error (conn_, r);
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
          translate_error (conn_, r);
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
    select_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      bool optimize,
                      binding& param,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize),
          result_ (result),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
        result_version_ = result_.version;
      }
    }

    select_statement::
    select_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      bool optimize,
                      binding& param,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize),
          result_ (result),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
        result_version_ = result_.version;
      }
    }

    select_statement::
    select_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      bool optimize,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize),
          result_ (result),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      if (!empty ())
      {
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
        result_version_ = result_.version;
      }
    }

    select_statement::
    select_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      bool optimize,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn,
                     text, statement_select,
                     (process ? &result : 0), optimize),
          result_ (result),
          lob_prefetch_size_ (lob_prefetch_size),
          done_ (true)
    {
      if (!empty ())
      {
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
        result_version_ = result_.version;
      }
    }

    void select_statement::
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
        translate_error (conn_, r);

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
      assert (n == result_count_);
#endif
    }

    select_statement::result select_statement::
    fetch ()
    {
      if (!done_)
      {
        change_callback* cc (result_.change_callback);

        if (cc != 0 && cc->callback != 0)
          (cc->callback) (cc->context, &result_);

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
          translate_error (conn_, r);
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
          translate_error (conn_, r);

        done_ = true;
      }
    }

    //
    // insert_statement
    //

    extern "C" sb4
    odb_oracle_returning_in (void* context,
                             OCIBind*,       // bind
                             ub4,            // iter
                             ub4,            // index
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

    extern "C" sb4
    odb_oracle_returning_out (void* context,
                              OCIBind*,        // bind
                              ub4,             // iter
                              ub4,             // index
                              void** buffer,
                              ub4** size,
                              ub1*,            // piece
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
    insert_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      binding& param,
                      bool returning)
        : statement (conn,
                     text, statement_insert,
                     (process ? &param : 0), false)
    {
      init (param, returning);
    }

    insert_statement::
    insert_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param,
                      bool returning)
        : statement (conn,
                     text, statement_insert,
                     (process ? &param : 0), false)
    {
      init (param, returning);
    }

    void insert_statement::
    init (binding& param, bool returning)
    {
      ub4 param_count (bind_param (param.bind, param.count));

      if (returning)
      {
        OCIError* err (conn_.error_handle ());
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               param_count + 1,
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
                            &odb_oracle_returning_in,
                            &id_bind_,
                            &odb_oracle_returning_out);

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
          translate_error (conn_, r);
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
    update_statement (connection_type& conn,
                      const string& text,
                      bool process,
                      binding& param)
        : statement (conn,
                     text, statement_update,
                     (process ? &param : 0), false)
    {
      if (!empty ())
        bind_param (param.bind, param.count);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param)
        : statement (conn,
                     text, statement_update,
                     (process ? &param : 0), false)
    {
      if (!empty ())
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
        translate_error (conn_, r);

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
    delete_statement (connection_type& conn,
                      const string& text,
                      binding& param)
        : statement (conn,
                     text, statement_delete,
                     0, false)
    {
      bind_param (param.bind, param.count);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* text,
                      binding& param)
        : statement (conn,
                     text, statement_delete,
                     0, false)
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
        translate_error (conn_, r);

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
