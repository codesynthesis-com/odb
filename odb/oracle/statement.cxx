// file      : odb/oracle/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <oci.h>

#include <odb/exceptions.hxx> // object_not_persistent

#include <odb/oracle/statement.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/error.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    static sb4
    param_callback_proxy (void* context,
                          OCIBind*,
                          ub4,            // iteration
                          ub4,            // index
                          void** buffer,
                          ub4* len,
                          ub1* piece,
                          void** indicator)
    {
      bind& b (*static_cast<bind*> (context));

      bool last (false), null (false);

      if (!(*b.param_callback) (&b.callback_context,
                                buffer,
                                len,
                                &null,
                                &last,
                                b.buffer,
                                b.capacity))
        return OCI_ROWCBK_DONE;


      *b.indicator = null ? -1 : 0;
      *indicator = &b.indicator;

      if (null)
        *piece = OCI_ONE_PIECE;
      else if (b.first_piece)
      {
        b.first_piece = false;
        *piece = last ? OCI_ONE_PIECE : OCI_FIRST_PIECE;
      }
      else if (last)
        *piece = OCI_LAST_PIECE;
      else
        *piece = OCI_NEXT_PIECE;

      return OCI_CONTINUE;
    }

    static sb4
    result_callback_proxy (void* context,
                           OCIDefine*,
                           ub4,                // iteration
                           void** buffer,
                           ub4** len,
                           ub1* piece,
                           void** indicator,
                           ub2**)              // return_code)
    {
      bind& b (*static_cast<bind*> (context));

      if (*piece == OCI_NEXT_PIECE)
        if (!(*b.result_callback) (&b.callback_context,
                                   &b.buffer,
                                   *reinterpret_cast<ub4*> (b.size),
                                   false,
                                   false))
          return OCI_ROWCBK_DONE;

      *buffer = b.buffer;
      *len = reinterpret_cast<ub4*> (b.size);
      **len = static_cast<ub4> (b.capacity);
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
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               err,
                               o,
                               b->buffer,
                               b->capacity,
                               b->type,
                               b->indicator,
                               b->size,
                               0,
                               0,
                               0,
                               b->param_callback != 0 ?
                               OCI_DATA_AT_EXEC : OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        if (b->param_callback != 0)
        {
          r = OCIBindDynamic (h, err, b, &param_callback_proxy, 0, 0);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
      }
    }

    void statement::
    bind_result (bind* b, size_t c)
    {
      OCIError* err (conn_.error_handle ());

      // The parameter position in OCIDefineByPos is specified as a 1-based
      // index.
      //
      for (size_t i (1); i <= c; ++i, ++b)
      {
        OCIDefine* h (0);

        sword r (OCIDefineByPos (stmt_,
                                 &h,
                                 err,
                                 i,
                                 b->buffer,
                                 b->capacity,
                                 b->type,
                                 b->indicator,
                                 b->size,
                                 0,
                                 b->result_callback != 0 ?
                                 OCI_DYNAMIC_FETCH : OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (err, r);

        if (b->result_callback != 0)
        {
          r = OCIDefineDynamic (h, err, b, &result_callback_proxy);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (err, r);
        }
      }
    }

    void statement::
    finalize_result (bind* b, std::size_t c)
    {
      for (size_t i (0); i < c; ++i)
      {
        if (b[i].result_callback != 0)
          (*b[i].result_callback) (&b[i].callback_context,
                                   b[i].buffer,
                                   *reinterpret_cast<ub4*> (b[i].size),
                                   *b[i].indicator == -1,
                                   true);
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
                      binding& data)
        : statement (conn, s),
          data_ (data),
          done_ (false)
    {
      bind_param (cond.bind, cond.count, 0);
      bind_result (data.bind, data.count);
    }

    void select_statement::
    execute ()
    {
      if (!done_)
        free_result ();

      // @@ Retrieve a single row into the already bound output buffers
      // as an optimization? This will avoid multiple server round-trips
      // in the case of a single object load. Remember to invoke
      // finalize_result on a successful prefetch.
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
        else
          finalize_result (data_.bind, data_.count);
      }

      return done_ ? no_data : success;
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
      b.indicator = -1;

      *buffer = 0;
      *len = 0;
      *piece = OCI_ONE_PIECE;
      *reinterpret_cast<sb2**> (indicator) = &b.indicator;

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
