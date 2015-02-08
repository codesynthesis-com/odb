// file      : odb/oracle/statement.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <cstring> // std::strlen, std::memset
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

    template <typename T>
    inline T*
    offset (T* base, size_t count, size_t size)
    {
      return reinterpret_cast<T*> (
        reinterpret_cast<char*> (base) + count * size);
    }

    extern "C" sb4
    odb_oracle_param_callback_proxy (void* context,
                                     OCIBind*,
                                     ub4 it,            // iteration
                                     ub4,               // index
                                     void** buffer,
                                     ub4* size,
                                     ub1* piece,
                                     void** indicator)
    {
      bind& b (*static_cast<bind*> (context));

      // Offset the data based on the current iteration and skip size (stored
      // in capacity).
      //
      sb2* ind (offset (b.indicator, it, b.capacity));

      // Only call the callback if the parameter is not NULL.
      //
      if (*ind != -1)
      {
        lob* l (static_cast<lob*> (offset (b.buffer, it, b.capacity)));
        lob_callback* cb (
          static_cast<lob_callback*> (offset (b.callback, it, b.capacity)));

        chunk_position pos;
        if (!(*cb->callback.param) (
              cb->context.param,
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

      *indicator = ind;

      return OCI_CONTINUE;
    }

    //
    // statement
    //

    statement::
    ~statement ()
    {
      if (stmt_ == 0)
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

      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
        {
          // Temporarily store the statement text in unbind data so that
          // text() which may be called by the tracer can access it.
          //
          udata_ = reinterpret_cast<unbind*> (const_cast<char*> (text));
          t->prepare (conn_, *this);
          udata_ = 0;
        }
      }

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
    }

    const char* statement::
    text () const
    {
      if (stmt_ == 0)
        // See init() above for details on what's going on here.
        //
        return udata_ != 0 ? reinterpret_cast<const char*> (udata_) : "";

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
    bind_param (bind* b, size_t n, size_t batch, size_t skip)
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

        // Unbind is only used in queries which means there should be no
        // batches.
        //
        assert (un == 0 || batch == 1);

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

        void* value (0);
        sb4 capacity;
        ub2* size (0);
        bool callback (b->callback != 0);

        switch (b->type)
        {
        case bind::timestamp:
          {
            for (size_t i (0); i != batch; ++i)
            {
              datetime* dt (
                static_cast<datetime*> (offset (b->buffer, i, skip)));

              void* pd (0); // Pointer to descriptor.

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
                  pd = &u.value;
                }
                else
                  pd = &dt->descriptor;

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
                pd = &dt->descriptor;

              if (i == 0)
                value = pd;
            }

            capacity = static_cast<sb4> (sizeof (OCIDateTime*));
            break;
          }
        case bind::interval_ym:
          {
            for (size_t i (0); i != batch; ++i)
            {
              interval_ym* iym (
                static_cast<interval_ym*> (offset (b->buffer, i, skip)));

              void* pd (0); // Pointer to descriptor.

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
                  pd = &u.value;
                }
                else
                  pd = &iym->descriptor;

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
                pd = &iym->descriptor;

              if (i == 0)
                value = pd;
            }

            capacity = static_cast<sb4> (sizeof (OCIInterval*));
            break;
          }
        case bind::interval_ds:
          {
            for (size_t i (0); i != batch; ++i)
            {
              interval_ds* ids (
                static_cast<interval_ds*> (offset (b->buffer, i, skip)));

              void* pd (0); // Pointer to descriptor.

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
                  pd = &u.value;
                }
                else
                  pd = &ids->descriptor;

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
                pd = &ids->descriptor;

              if (i == 0)
                value = pd;
            }

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
              for (size_t i (0); i != batch;)
              {
                l->buffer = &lob_buffer;
                l = static_cast<lob*> (offset (b->buffer, ++i, skip));
              }
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

            // Store skip in capacity so that the callback can offset the
            // values based on the iteration number.
            //
            b->capacity = static_cast<ub4> (skip);

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

        // Set array information if we have a batch.
        //
        if (batch != 1)
        {
          ub4 s (static_cast<ub4> (skip));

          r = OCIBindArrayOfStruct (h,
                                    err,
                                    (value != 0 ? s : 0),        // value
                                    (b->indicator != 0 ? s : 0), // indicator
                                    (size != 0 ? s : 0),         // length
                                    0);                          // return code

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
              l->environment = env;
              l->error = err;
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

        // LOB prefetching is only supported in OCI version 11.1 and greater
        // and in Oracle server 11.1 and greater. If this code is called
        // against a pre 11.1 server, the call to OCIAttrSet will return an
        // error code.
        //
#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >= 1) \
  || OCI_MAJOR_VERSION > 11
        if (b->type == bind::blob ||
            b->type == bind::clob ||
            b->type == bind::nclob)
        {

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
        }
        else
#endif
        if (b->type == bind::nstring)
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
    // bulk_statement
    //

    bulk_statement::
    ~bulk_statement () {}

    sword bulk_statement::
    execute (size_t n, multiple_exceptions* mex, sb4 ignore_code)
    {
      {
        odb::tracer* t;
        if ((t = conn_.transaction_tracer ()) ||
            (t = conn_.tracer ()) ||
            (t = conn_.database ().tracer ()))
          t->execute (conn_, *this);
      }

      mex_ = mex;

      OCIError* err (conn_.error_handle ());

      // We use OCI_BATCH_ERRORS for n == 1 in order to get the batch
      // error reporting even for a single parameter set. This makes
      // it easier to populate mex since otherwise we would have two
      // cases to worry about: batch and non-batch (statement fails
      // as a whole).
      //
      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               err,
                               static_cast<ub4> (n),
                               0,
                               0,
                               0,
                               status_ == 0 ? OCI_DEFAULT : OCI_BATCH_ERRORS));

      // If the statement failed as a whole, assume no parameter sets
      // were attempted in case of a batch. Otherwise, in the batch
      // errors mode, all the sets are always attempted (let's hope
      // this is actually true).
      //
      i_ = 0;
      n_ = (r == OCI_ERROR || r == OCI_INVALID_HANDLE
            ? (status_ == 0 ? 1 : 0)
            : n);

      if (mex_ != 0)
      {
        mex_->current (i_);
        mex_->attempted (n_);
      }

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
      {
        if (mex_ != 0)
          mex_->fatal (true); // An incomplete batch is always fatal.

        return r;
      }

      // Initialize the batch status array and extract error information
      // for failed parameter sets.
      //
      if (status_ != 0)
      {
        sword r; // Our own return code.

        // Clear the status array.
        //
        memset (status_, 0, n * sizeof (status_[0]));

        if (err1_ == 0)
        {
          OCIError* e (0);
          r = OCIHandleAlloc (conn_.database ().environment (),
                              reinterpret_cast<void**> (&e),
                              OCI_HTYPE_ERROR,
                              0,
                              0);

          if (r != OCI_SUCCESS)
            throw invalid_oci_handle ();

          err1_.reset (e);
        }

        ub4 errors;
        r = OCIAttrGet (stmt_,
                        OCI_HTYPE_STMT,
                        &errors,
                        0,
                        OCI_ATTR_NUM_DML_ERRORS,
                        err1_);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err1_, r);

        errors_ = errors;

        if (errors != 0)
        {
          auto_handle<OCIError> err2;

          {
            OCIError* e (0);
            r = OCIHandleAlloc (conn_.database ().environment (),
                                reinterpret_cast<void**> (&e),
                                OCI_HTYPE_ERROR,
                                0,
                                0);

            if (r != OCI_SUCCESS)
              throw invalid_oci_handle ();

            err2.reset (e);
          }

          for (ub4 i (0); i != errors; ++i)
          {
            {
              OCIError* tmp (err2);
              r = OCIParamGet (err,                             // from
                               OCI_HTYPE_ERROR,
                               err1_,                            // diagnostics
                               reinterpret_cast<void**> (&tmp), // to
                               i);

              if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
                translate_error (err1_, r);
            }

            ub4 row;
            r = OCIAttrGet (err2,
                            OCI_HTYPE_ERROR,
                            &row,
                            0,
                            OCI_ATTR_DML_ROW_OFFSET,
                            err1_);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err1_, r);

            OCIErrorGet (err2, 1, 0, &status_[row], 0, 0, OCI_HTYPE_ERROR);

            if (status_[row] != ignore_code)
              translate_error (err2, OCI_ERROR, &conn_, row, mex_);
          }
        }
      }

      return r;
    }

    unsigned long long bulk_statement::
    affected (bool unique)
    {
      unsigned long long rows;
      {
        ub4 n (0);
        OCIError* err (conn_.error_handle ());
        sword r (OCIAttrGet (stmt_,
                             OCI_HTYPE_STMT,
                             &n,
                             0,
                             OCI_ATTR_ROW_COUNT,
                             err));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        rows = static_cast<unsigned long long> (n);
      }

      if (n_ > 1) // Batch.
      {
        if (rows != 0) // Some rows did get affected.
        {
          // Subtract the parameter sets that failed since they haven't
          // affected any rows.
          //
          size_t p (n_ - errors_);

          if (p > 1) // True batch.
          {
            if (unique) // Each can affect 0 or 1 row.
            {
              rows = (p == static_cast<size_t> (rows)
                      ? 1
                      : result_unknown);
            }
            else
              rows = result_unknown;
          }
        }
      }

      return rows;
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
          done_ (true)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
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
          done_ (true)
    {
      if (!empty ())
      {
        bind_param (param.bind, param.count);
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
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
          done_ (true)
    {
      if (!empty ())
      {
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
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
          done_ (true)
    {
      if (!empty ())
      {
        result_count_ = bind_result (
          result.bind, result.count, lob_prefetch_size);
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
          (cc->callback) (cc->context);

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
                             ub4 it,         // iter
                             ub4,            // index
                             void** buffer,
                             ub4* size,
                             ub1* piece,
                             void** indicator)
    {
      binding& ret (*static_cast<insert_statement*> (context)->ret_);

      // Offset the data based on the current iteration and skip size.
      // The id is always first.
      //
      *buffer = 0;
      *size = 0;
      *piece = OCI_ONE_PIECE;

      sb2* ind (offset (ret.bind[0].indicator, it, ret.skip));
      *ind = -1;
      *indicator = ind;

      return OCI_CONTINUE;
    }

    extern "C" sb4
    odb_oracle_returning_out (void* context,
                              OCIBind*,        // bind
                              ub4 it,          // iter
                              ub4,             // index
                              void** buffer,
                              ub4** size,
                              ub1* piece,
                              void** indicator,
                              ub2** rcode)
    {
      insert_statement& st (*static_cast<insert_statement*> (context));
      bind& b (st.ret_->bind[0]); // The id is always first.
      size_t skip (st.ret_->skip);

      // Offset the data based on the current iteration and skip size.
      //
      *buffer = offset (b.buffer, it, skip);

      if (b.type == bind::number)
      {
        // So the straightforward way to handle this would have been to
        // set size to the properly offset pointer to b.size, just like
        // we do for the buffer and indicator. The problem is that in
        // OCI size is ub2 everywhere except in the *Dynamic() callbacks.
        // Here it is expected to be ub4 and, as a result, we cannot use
        // our ub2 size that we use throughout (I know you are tempted
        // to just cast ub2* to ub4* and forget about this mess, but,
        // trust me, this won't end up well).
        //
        // So what we will do instead is this: have a temporary ub4 buffer
        // that we return to OCI so that it can store the size for us. But
        // the callback can be called multiple times (batch operations) so
        // on each subsequent call we will have to save the size from the
        // previous call into our ub2 array. We will also have to handle
        // the last extracted size after OCIStmtExecute() below. Thanks,
        // Oracle!
        //
        if (st.ret_prev_ != 0)
          *st.ret_prev_ = static_cast<ub2> (st.ret_size_);

        st.ret_prev_ = offset (b.size, it, skip);
        *size = &st.ret_size_;
      }

      // For some reason we have to set the out size to the (presumably)
      // maximum buffer size.
      //
      **size = b.capacity;

      *indicator = offset (b.indicator, it, skip);
      *rcode = 0;
      *piece = OCI_ONE_PIECE;

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
                      binding* returning)
        : bulk_statement (conn,
                          text, statement_insert,
                          (process ? &param : 0), false,
                          param.batch, param.status),
          ret_ (returning)
    {
      init (param);
    }

    insert_statement::
    insert_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param,
                      binding* returning)
        : bulk_statement (conn,
                          text, statement_insert,
                          (process ? &param : 0), false,
                          param.batch, param.status),
          ret_ (returning)
    {
      init (param);
    }

    void insert_statement::
    init (binding& param)
    {
      ub4 param_count (bind_param (param.bind, param.count,
                                   param.batch, param.skip));
      if (ret_ != 0)
      {
        OCIError* err (conn_.error_handle ());
        OCIBind* h (0);

        bind* b (ret_->bind);

#if OCI_MAJOR_VERSION < 11 ||                           \
  (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION < 2)
        // Assert if a 64 bit integer buffer type is provided and the OCI
        // version is unable to implicitly convert the NUMBER binary data
        // to the relevant type.
        //
        assert ((b->type != bind::integer && b->type != bind::uinteger) ||
                b->capacity <= 4);
#endif
        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               param_count + 1,
                               0,
                               b->capacity,
                               param_sqlt_lookup[b->type],
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
                            this,
                            &odb_oracle_returning_in,
                            this,
                            &odb_oracle_returning_out);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);
      }
    }

    size_t insert_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      OCIError* err (conn_.error_handle ());

      if (ret_ != 0)
        ret_prev_ = 0;

      // Ignore ORA-00001 error code, see fetch() below for details.
      //
      sword r (bulk_statement::execute (n, mex, (ret_ == 0 ? 1 : 0)));

      // Statement failed as a whole, assume no parameter sets were
      // attempted in case of a batch.
      //
      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
      {
        sb4 e;
        OCIErrorGet (err, 1, 0, &e, 0, 0, OCI_HTYPE_ERROR);
        fetch (r, e);

        if (result_) // If fetch() hasn't translated the error.
          translate_error (err, r, &conn_, 0, mex_); // Can return.

        return n_;
      }

      // Store the last returned id size (see odb_oracle_returning_out()
      // for details).
      //
      if (ret_ != 0 && ret_prev_ != 0)
        *ret_prev_ = static_cast<ub2> (ret_size_);

      if (status_ == 0) // Non-batch mode.
        fetch (OCI_SUCCESS, 0);
      else
      {
        fetch (status_[i_] == 0 ? OCI_SUCCESS : OCI_ERROR, status_[i_]);
      }

      return n_;
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
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.status),
          unique_ (false)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.

      if (!empty ())
        bind_param (param.bind, param.count, param.batch, param.skip);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const string& text,
                      bool unique,
                      bool process,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.status),
          unique_ (unique)
    {
      if (!empty ())
        bind_param (param.bind, param.count, param.batch, param.skip);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* text,
                      bool process,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.status),
          unique_ (false)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.

      if (!empty ())
        bind_param (param.bind, param.count, param.batch, param.skip);
    }

    update_statement::
    update_statement (connection_type& conn,
                      const char* text,
                      bool unique,
                      bool process,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_update,
                          (process ? &param : 0), false,
                          param.batch, param.status),
          unique_ (unique)
    {
      if (!empty ())
        bind_param (param.bind, param.count, param.batch, param.skip);
    }

    size_t update_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      OCIError* err (conn_.error_handle ());
      sword r (bulk_statement::execute (n, mex));

      // Statement failed as a whole, assume no parameter sets were
      // attempted in case of a batch.
      //
      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
      {
        translate_error (err, r, &conn_, 0, mex_); // Can return.
        return n_;
      }

      // Figure out the affected (matched, not necessarily updated)
      // row count.
      //
      result_ = affected (unique_);

      return n_;
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
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.status),
          unique_ (false)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.
      bind_param (param.bind, param.count, param.batch, param.skip);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const string& text,
                      bool unique,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.status),
          unique_ (unique)
    {
      bind_param (param.bind, param.count, param.batch, param.skip);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* text,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.status),
          unique_ (false)
    {
      assert (param.batch == 1); // Specify unique_hint explicitly.
      bind_param (param.bind, param.count, param.batch, param.skip);
    }

    delete_statement::
    delete_statement (connection_type& conn,
                      const char* text,
                      bool unique,
                      binding& param)
        : bulk_statement (conn,
                          text, statement_delete,
                          0, false,
                          param.batch, param.status),
          unique_ (unique)
    {
      bind_param (param.bind, param.count, param.batch, param.skip);
    }

    size_t delete_statement::
    execute (size_t n, multiple_exceptions* mex)
    {
      sword r (bulk_statement::execute (n, mex));
      OCIError* err (conn_.error_handle ());

      // Statement failed as a whole, assume no parameter sets were
      // attempted in case of a batch.
      //
      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
      {
        translate_error (err, r, &conn_, 0, mex_); // Can return.
        return n_;
      }

      // Figure out the affected  row count.
      //
      result_ = affected (unique_);

      return n_;
    }
  }
}
