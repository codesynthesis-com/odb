// file      : odb/oracle/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

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
      SQLT_VNU,       // bind::number
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
      SQLT_VNU,       // bind::number
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

      // Only call the callback if the parameter is not NULL.
      //
      if (*b.indicator != -1)
      {
        chunk_position pos;
        if (!(*b.callback.param) (&b.callback_context,
                                  reinterpret_cast<ub4*> (b.size),
                                  const_cast<const void**> (buffer),
                                  size,
                                  &pos,
                                  b.buffer,
                                  b.capacity))
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
    bind_param (bind* b, size_t c, size_t o)
    {
      OCIError* err (conn_.error_handle ());

      // The parameter position in OCIBindByPos is specified as a 1-based
      // index. Convert o to a 1-based offset.
      //
      ++o;

      for (size_t e (o + c); o < e; ++o, ++b)
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
                               o,
                               callback ? 0 : b->buffer,
                               static_cast<sb4> (b->capacity),
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
          // When binding a LOB result, the bind::size member is reinterpreted
          // as a pointer to an auto_descriptor<OCILobLocator>. If the
          // descriptor has not yet been allocated, it is allocated now.
          //
          auto_descriptor<OCILobLocator>* lob (
            reinterpret_cast<auto_descriptor<OCILobLocator>*> (b->size));

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

          // LOB prefetching is only supported in OCI version 11.1 and greater
          // and in Oracle server 11.1 and greater. If this code is called
          // against a pre 11.1 server, the call to OCIAttrSet will return an
          // error code.
          //
#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >= 1) \
  || OCI_MAJOR_VERSION > 11
          if (p != 0)
          {
            r = OCIAttrSet (h,
                            OCI_HTYPE_DEFINE,
                            &p,
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
            *b->indicator != -1 &&
            b->callback.result != 0)
        {
          // If b->capacity is 0, we will be stuck in an infinite loop.
          //
          assert (b->capacity != 0);

          auto_descriptor<OCILobLocator>& locator (
            *reinterpret_cast<auto_descriptor<OCILobLocator>* > (b->size));

          ub1 piece (OCI_FIRST_PIECE);

          // Setting the value pointed to by the byte_amt argument to 0 on the
          // first call to OCILobRead2 instructs OCI to remain in a polling
          // state until the EOF is reached, at which point OCILobRead2 will
          // return OCI_SUCCESS.
          //
          ub8 read (0);
          ub1 cs_form (b->type == bind::nclob ? SQLCS_NCHAR : SQLCS_IMPLICIT);

          sword r;
          do
          {
            r = OCILobRead2 (conn_.handle (),
                             err,
                             locator,
                             &read,
                             0,
                             1,
                             b->buffer,
                             b->capacity,
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
            if (!(*b->callback.result) (b->callback_context,
                                        b->buffer,
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
                      binding& cond,
                      binding& data,
                      size_t lob_prefetch_size)
        : statement (conn, s),
          data_ (data),
          done_ (true)
    {
      bind_param (cond.bind, cond.count, 0);
      bind_result (data.bind, data.count, lob_prefetch_size);
    }

    select_statement::
    select_statement (connection& conn,
                      const string& s,
                      binding& data,
                      size_t lob_prefetch_size)
        : statement (conn, s),
          data_ (data),
          done_ (true)
    {
      bind_result (data.bind, data.count, lob_prefetch_size);
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
      assert (n == data_.count);
#endif

      done_ = false;
    }

    select_statement::result select_statement::
    fetch ()
    {
      if (!done_)
      {
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
                      binding& data,
                      bool returning)
        : statement (conn, s)
    {
      bind_param (data.bind, data.count, 0);

      if (returning)
      {
        OCIError* err (conn_.error_handle ());
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               data.count,
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
                      binding& cond,
                      binding& data)
        : statement (conn, s)
    {
      bind_param (data.bind, data.count, 0);
      bind_param (cond.bind, cond.count, data.count);
    }

    void update_statement::
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
      if (row_count == 0)
        throw object_not_persistent ();
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
                      binding& cond)
        : statement (conn, s)
    {
      bind_param (cond.bind, cond.count, 0);
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
