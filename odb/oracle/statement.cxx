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
#include <odb/oracle/error.hxx>
#include <odb/oracle/auto-descriptor.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    static sb4
    param_callback_proxy (void* context,
                          OCIBind*,
                          ub4,               // iteration
                          ub4,               // index
                          void** buffer,
                          ub4* len,
                          ub1* piece,
                          void** indicator)
    {
      bind& b (*static_cast<bind*> (context));

      // Only callback to user if the parameter is not NULL.
      //
      if (*b.indicator != -1)
      {
        bind::piece p;

        if (!(*b.callback.param) (&b.callback_context,
                                  buffer,
                                  len,
                                  reinterpret_cast<ub4*> (b.size),
                                  &p,
                                  b.buffer,
                                  b.capacity))
          return OCI_ERROR;

        switch (p)
        {
        case bind::whole:
          {
            *piece = OCI_ONE_PIECE;
            break;
          }
        case bind::first:
          {
            *piece = OCI_FIRST_PIECE;
            break;
          }
        case bind::next:
          {
            *piece = OCI_NEXT_PIECE;
            break;
          }
        case bind::last:
          {
            *piece = OCI_LAST_PIECE;
            break;
          }
        default:
          assert (0);
          return OCI_ERROR;
        }
      }
      else
        *piece = OCI_ONE_PIECE;

      *indicator = &b.indicator;

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
      // index. Convert 'o' to a 1-based offset.
      //
      ++o;

      for (size_t e (o + c); o < e; ++c, ++b)
      {
        bool callback ((b->type == SQLT_BLOB || b->type == SQLT_CLOB) &&
                       b->callback.param != 0);

        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               o,
                               b->buffer,
                               static_cast<sb4> (b->capacity),
                               b->type,
                               b->indicator,
                               b->size,
                               0,
                               0,
                               0,
                               callback ? OCI_DATA_AT_EXEC : OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

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

      // The parameter position in OCIDefineByPos is specified as a 1-based
      // index.
      //
      for (size_t i (1); i <= c; ++i, ++b)
      {
        if (b->type == SQLT_BLOB || b->type == SQLT_CLOB)
        {
          OCIDefine* h (0);
          sword r (OCIDefineByPos (stmt_,
                                   &h,
                                   err,
                                   i,
                                   reinterpret_cast<OCILobLocator*> (b->size),
                                   sizeof (OCILobLocator*),
                                   b->type,
                                   b->indicator,
                                   0,
                                   0,
                                   OCI_DEFAULT));

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);

          // @@ LOB prefetching is only supported in OCI version 11.1 and
          // greater and in Oracle server 11.1 and greater. If this code is
          // called against a pre 11.1 server, the call to OCIAttrSet will
          // return an error code.
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

            bool b (true);
            r = OCIAttrSet (h,
                            OCI_HTYPE_DEFINE,
                            &b,
                            0,
                            OCI_ATTR_LOBPREFETCH_LENGTH,
                            err);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err, r);
          }
#endif
        }
        else
        {
          OCIDefine* h (0);
          sword r (OCIDefineByPos (stmt_,
                                   &h,
                                   err,
                                   i,
                                   b->buffer,
                                   static_cast<sb4> (b->capacity),
                                   b->type,
                                   b->indicator,
                                   b->size,
                                   0,
                                   OCI_DEFAULT));

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
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
                      std::size_t lob_prefetch_len)
        : statement (conn, s),
          data_ (data),
          done_ (false)
    {
      bind_param (cond.bind, cond.count, 0);
      bind_result (data.bind, data.count, lob_prefetch_len);
    }

    void select_statement::
    execute ()
    {
      if (!done_)
        free_result ();

      // @@ Retrieve a single row into the already bound output buffers
      // as an optimization? This will avoid multiple server round-trips
      // in the case of a single object load. Keep in mind the implications
      // on LOB prefetch.
      //
      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               conn_.error_handle (),
                               0,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);
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

    void select_statement::
    stream_result_lobs ()
    {
      OCIError* err (conn_.error_handle());

      for (size_t i (0); i < data_.count; ++i)
      {
        // Only stream if the bind specifies a LOB type, and the LOB
        // value is not NULL, and a result callback has been provided.
        //
        if ((data_.bind[i].type == SQLT_BLOB ||
             data_.bind[i].type == SQLT_CLOB) &&
            *data_.bind[i].indicator != -1 &&
            data_.bind[i].callback.result != 0)
        {
          // @@ If data_.bind[i].capacity is 0, we will be stuck in an
          // infinite loop.
          //
          assert (data_.bind[i].capacity != 0);

          OCILobLocator* locator (
            reinterpret_cast<OCILobLocator*> (data_.bind[i].size));

          ub8 length (0);
          sword r (OCILobGetLength2(conn_.handle (),
                                    err,
                                    locator,
                                    &length));

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);

          for (ub8 total (0), read (0); total < length; total += read)
          {
            read = length - total;
            read = read > data_.bind[i].capacity ?
              data_.bind[i].capacity : read;

            // The call to OCILobRead2 does not need to know when the last
            // piece is being requested.
            //
            ub1 oci_piece (total == 0 ? OCI_FIRST_PIECE : OCI_NEXT_PIECE);

            r = OCILobRead2 (conn_.handle (),
                             err,
                             locator,
                             &read,
                             0,
                             1,  // Starting offset. The first position is 1.
                                 // This parameter is only used by OCI on the
                                 // first call when polling.
                             data_.bind[i].buffer,
                             data_.bind[i].capacity,
                             oci_piece,
                             0,
                             0,
                             0,
                             SQLCS_IMPLICIT);

            if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
              translate_error (err, r);

            bind::piece user_piece;
            if (oci_piece == OCI_FIRST_PIECE)
              user_piece = read == length ? bind::whole : bind::first;
            else
              user_piece = total + read < length ? bind::next : bind::last;

            if (!(*data_.bind[i].callback.result) (
                  data_.bind[i].callback_context,
                  data_.bind[i].buffer,
                  read,
                  user_piece))
              break;
          }
        }
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
                  ub4* len,
                  ub1* piece,
                  void** indicator)
    {
      typedef insert_statement::id_bind_type bind;

      bind& b (*static_cast<bind*> (context));

      *buffer = 0;
      *len = 0;
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
                   ub4** len,
                   ub1*,               // piece
                   void** indicator,
                   ub2** rcode)
    {
      typedef insert_statement::id_bind_type bind;

      bind& b (*static_cast<bind*> (context));

#if (OCI_MAJOR_VERSION == 11 && OCI_MINOR_VERSION >=2) \
  || OCI_MAJOR_VERSION > 11
      *buffer = &b.id.value64;
      **len = sizeof (unsigned long long);
#else
      *buffer = &b.id.value32;
      **len = sizeof (unsigned int);
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
