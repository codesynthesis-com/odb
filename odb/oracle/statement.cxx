// file      : odb/oracle/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>
#include <limits>

#include <oci.h>

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
    static ub4 param_sqlt_lookup[bind::last] =
    {
      SQLT_INT,       // bind::integer
      SQLT_UIN,       // bind::uinteger
      SQLT_BFLOAT,    // bind::binary_float
      SQLT_BDOUBLE,   // bind::binary_double
      SQLT_NUM,       // bind::number
      SQLT_DAT,       // bind::date
      SQLT_TIMESTAMP, // bind::timestamp
      SQLT_CHR,       // bind::string
      SQLT_CHR,       // bind::nstring
      SQLT_BIN,       // bind::raw
      SQLT_LBI,       // bind::blob
      SQLT_LNG,       // bind::clob
      SQLT_LNG        // bind::nclob
    };

    // Mapping of bind::buffer_type values for result buffers to their
    // equivalent external OCI typecode identifiers.
    //
    static ub4 result_sqlt_lookup[bind::last] =
    {
      SQLT_INT,       // bind::integer
      SQLT_UIN,       // bind::uinteger
      SQLT_BFLOAT,    // bind::binary_float
      SQLT_BDOUBLE,   // bind::binary_double
      SQLT_NUM,       // bind::number
      SQLT_DAT,       // bind::date
      SQLT_TIMESTAMP, // bind::timestamp
      SQLT_CHR,       // bind::string
      SQLT_CHR,       // bind::nstring
      SQLT_BIN,       // bind::raw
      SQLT_BLOB,      // bind::blob
      SQLT_CLOB,      // bind::clob
      SQLT_CLOB       // bind::nclob
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
        reinterpret_cast<details::buffer*>(b.buffer));

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
    statement (connection& conn, const string& s)
        : conn_ (conn)
    {
      OCIError* err (conn_.error_handle ());
      OCIStmt* handle (0);

      sword r (OCIStmtPrepare2 (conn_.handle (),
                                &handle,
                                err,
                                reinterpret_cast<const text*> (s.c_str ()),
                                static_cast<ub4> (s.size ()),
                                0,
                                0,
                                OCI_NTV_SYNTAX,
                                OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (err, r);

      stmt_.reset (handle, OCI_STRLS_CACHE_DELETE, err);
    }

    void statement::
    bind_param (bind* b, size_t n)
    {
      OCIError* err (conn_.error_handle ());

      // The parameter position in OCIBindByPos is specified as a 1-based
      // index.
      //
      n++;
      for (ub4 i (1); i < n; ++i, ++b)
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

        bool callback (b->callback != 0);
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               i,
                               callback ? 0 : b->buffer,
                               // When parameter callbacks are in use, set the
                               // allowable data length to the maximum
                               // possible.
                               //
                               callback
                                 ? std::numeric_limits<sb4>::max ()
                                 : static_cast<sb4> (b->capacity),
                               param_sqlt_lookup[b->type],
                               callback ? 0 : b->indicator,
                               callback ? 0 : b->size,
                               0,
                               0,
                               0,
                               callback ? OCI_DATA_AT_EXEC : OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

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
          details::buffer& lob_buffer (conn_.lob_buffer ());

          if (lob_buffer.capacity () == 0)
            lob_buffer.capacity (4096);

          b->buffer = &lob_buffer;

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

      OCIError* err (conn_.error_handle ());

      for (size_t i (1); i <= c; ++i, ++b)
      {
        OCIDefine* h (0);

        if (b->type == bind::blob ||
            b->type == bind::clob ||
            b->type == bind::nclob)
        {
          // When binding a LOB result, the bind::buffer member is
          // reinterpreted as a pointer to an auto_descriptor<OCILobLocator>.
          // If the descriptor has not yet been allocated, it is allocated now.
          //
          auto_descriptor<OCILobLocator>* lob (
            reinterpret_cast<auto_descriptor<OCILobLocator>*> (b->buffer));

          if (lob->get () == 0)
          {
            OCILobLocator* h (0);

            sword r (OCIDescriptorAlloc (conn_.database ().environment (),
                                         reinterpret_cast<void**> (&h),
                                         OCI_DTYPE_LOB,
                                         0,
                                         0));

            // OCIDescriptorAlloc will return OCI_SUCCESS on success, or
            // OCI_INVALID_HANDLE on an out-of-memory condition.
            //
            if (r != OCI_SUCCESS)
              throw invalid_oci_handle ();

            lob->reset (h);
          }

          sword r (OCIDefineByPos (stmt_,
                                   &h,
                                   err,
                                   i,
                                   lob,
                                   sizeof (OCILobLocator*),
                                   result_sqlt_lookup[b->type],
                                   b->indicator,
                                   0,
                                   0,
                                   OCI_DEFAULT));

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);

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
        else
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

          sword r (OCIDefineByPos (stmt_,
                                   &h,
                                   err,
                                   i,
                                   b->buffer,
                                   static_cast<sb4> (b->capacity),
                                   result_sqlt_lookup[b->type],
                                   b->indicator,
                                   b->size,
                                   0,
                                   OCI_DEFAULT));

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);

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
      }
    }

    void statement::
    rebind_result (bind* b, size_t c, size_t p)
    {
      ODB_POTENTIALLY_UNUSED (p);

      OCIError* err (conn_.error_handle ());

      for (size_t i (1); i <= c; ++i, ++b)
      {
        if (!(b->type == bind::blob ||
              b->type == bind::clob ||
              b->type == bind::nclob))
          continue;

        // When binding a LOB result, the bind::buffer member is
        // reinterpreted as a pointer to an auto_descriptor<OCILobLocator>.
        // If the descriptor has been reset, it is re-allocated now.
        //
        auto_descriptor<OCILobLocator>* lob (
          reinterpret_cast<auto_descriptor<OCILobLocator>*> (b->buffer));

        if (lob->get () == 0)
        {
          OCILobLocator* h (0);

          sword r (OCIDescriptorAlloc (conn_.database ().environment (),
                                       reinterpret_cast<void**> (&h),
                                       OCI_DTYPE_LOB,
                                       0,
                                       0));

          // OCIDescriptorAlloc will return OCI_SUCCESS on success, or
          // OCI_INVALID_HANDLE on an out-of-memory condition.
          //
          if (r != OCI_SUCCESS)
            throw invalid_oci_handle ();

          lob->reset (h);
        }

        OCIDefine* h (reinterpret_cast<OCIDefine*> (b->size));

        sword r (OCIDefineByPos (stmt_,
                                 &h,
                                 err,
                                 i,
                                 lob,
                                 sizeof (OCILobLocator*),
                                 result_sqlt_lookup[b->type],
                                 b->indicator,
                                 0,
                                 0,
                                 OCI_DEFAULT));

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
    }

    void statement::
    stream_result (bind* b, size_t c)
    {
      OCIError* err (conn_.error_handle());

      for (size_t i (0); i < c; ++i, ++b)
      {
        // Only stream if the bind specifies a LOB type, and the LOB value is
        // not NULL, and a result callback has been provided.
        //
        if ((b->type == bind::blob ||
             b->type == bind::clob ||
             b->type == bind::nclob) &&
            *b->indicator != -1 && b->callback->result != 0)
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

    statement::
    ~statement ()
    {
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
                      const string& s,
                      binding& param,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn, s),
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
                      const string& s,
                      binding& result,
                      size_t lob_prefetch_size)
        : statement (conn, s),
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
      r = OCIAttrGet(stmt_, OCI_HTYPE_STMT, &n, 0, OCI_ATTR_PARAM_COUNT, err);

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
      *buffer = &b.id.value64;
      **size = sizeof (unsigned long long);
#else
      *buffer = &b.id.value32;
      **size = sizeof (unsigned int);
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
                      const string& s,
                      binding& param,
                      bool returning)
        : statement (conn, s)
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
#else
                               sizeof (unsigned int),
#endif
                               SQLT_UIN,
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
      return id_bind_.id.value64;
#else
      return id_bind_.id.value32;
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
    update_statement (connection& conn,
                      const string& s,
                      binding& param)
        : statement (conn, s)
    {
      bind_param (param.bind, param.count);
    }

    unsigned long long update_statement::
    execute ()
    {
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
    delete_statement (connection& conn,
                      const string& s,
                      binding& param)
        : statement (conn, s)
    {
      bind_param (param.bind, param.count);
    }

    unsigned long long delete_statement::
    execute ()
    {
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
