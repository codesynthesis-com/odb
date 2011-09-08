// file      : odb/oracle/statement.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#if OCI_MAJOR_VERSION >= 11 && OCI_MINOR_VERSION >= 2
#  define ODB_ORACLE_USE_64_BIT_ID
#endif

#include <odb/exceptions.hxx> // object_not_persistent

#include <odb/oracle/statement.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/error.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    //
    // statement
    //

    statement::
    statement (connection& conn, const string& s)
        : conn_ (conn)
    {
      OCIStmt* handle (0);

      sword r (OCIStmtPrepare2 (conn_.handle (),
                                &handle,
                                conn_.error_handle (),
                                reinterpret_cast<const text*> (s.c_str ()),
                                static_cast<ub4> (s.size ()),
                                0,
                                0,
                                OCI_NTV_SYNTAX,
                                OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

      stmt_.reset (handle, OCI_DEFAULT, conn_.error_handle ());
    }

    void statement::
    bind_param (bind* b, size_t c, size_t o)
    {
      for (size_t i (0); i < c; ++i)
      {
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               conn_.error_handle (),
                               o + i,
                               b[i].buffer,
                               b[i].capacity,
                               b[i].type,
                               b[i].indicator,
                               b[i].size,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (conn_.error_handle (), r);
      }
    }

    void statement::
    bind_result (bind* b, size_t c)
    {
      for (size_t i (0); i < c; ++i)
      {
        OCIDefine* h (0);

        sword r (OCIDefineByPos (stmt_,
                                 &h,
                                 conn_.error_handle (),
                                 i,
                                 b[i].buffer,
                                 b[i].capacity,
                                 b[i].type,
                                 b[i].indicator,
                                 b[i].size,
                                 0,
                                 OCI_DEFAULT));

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (conn_.error_handle (), r);
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
          end_ (false),
          rows_ (0)
    {
      bind_param (cond.bind, cond.count, 0);
      bind_result (data.bind, data.count);
    }

    void select_statement::
    execute ()
    {
      if (!end_)
        free_result ();

      // @@ Retrieve a single row into the already bound output buffers
      // as an optimization? This will avoid multiple server round-trips
      // in the case of a single object load.
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
      sword r (OCIStmtFetch2 (stmt_,
                              conn_.error_handle (),
                              1,
                              OCI_FETCH_NEXT,
                              0,
                              OCI_DEFAULT));

      switch (r)
      {
      case OCI_SUCCESS:
        {
          ++rows_;
          return success;
        }
      case OCI_NO_DATA:
        {
          end_ = true;
          return no_data;
        }
      default:
        {
          translate_error (conn_.error_handle (), r);
          return no_data; // Never reached.
        }
      }
    }


    void select_statement::
    free_result ()
    {
      end_ = true;
      rows_ = 0;

      sword r (OCIStmtFetch2 (stmt_,
                              conn_.error_handle (),
                              0,
                              OCI_FETCH_NEXT,
                              0,
                              OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);
    }

    //
    // insert_statement
    //

    static sb4
    returning_in_cb (void* context,
                     OCIBind*,            // bind
                     ub4,                 // iter
                     ub4,                 // index
                     void** buffer,
                     ub4* len,
                     ub1* piece,
                     void** indicator)
    {
      typedef insert_statement::id_bind_type bind;

      bind& b (*reinterpret_cast<bind*> (context));
      b.ind = -1;

      *buffer = 0;
      *len = 0;
      *piece = OCI_ONE_PIECE;
      *reinterpret_cast<sb2**> (indicator) = &b.ind;

      return OCI_CONTINUE;
    }

    static sb4
    returning_out_cb (void* context,
                      OCIBind*,           // bind
                      ub4,                // iter
                      ub4,                // index
                      void** buffer,
                      ub4** len,
                      ub1*,               // piece
                      void** ind,
                      ub2** rcode)
    {
      typedef insert_statement::id_bind_type bind;

      bind& b (*reinterpret_cast<bind*> (context));

#ifdef ODB_ORACLE_USE_64_BIT_ID
      *buffer = &b.id.value_64;
      **len = sizeof (unsigned long long);
#else
      *buffer = &b.id.value_32;
      **len = sizeof (unsigned int);
#endif

      *ind = reinterpret_cast<void*> (&b.ind);
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
        OCIBind* h (0);

        sword r (OCIBindByPos (stmt_,
                               &h,
                               conn_.error_handle (),
                               data.count,
                               0,
#ifdef ODB_ORACLE_USE_64_BIT_ID
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
          translate_error (conn_.error_handle (), r);

        r = OCIBindDynamic (h,
                            conn_.error_handle (),
                            reinterpret_cast<void*> (&id_bind_),
                            &returning_in_cb,
                            reinterpret_cast<void*> (&id_bind_),
                            &returning_out_cb);
      }
    }

    bool insert_statement::
    execute ()
    {
      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               conn_.error_handle (),
                               1,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      conn_.error_handle ());

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

      // The value of the OCI_ATTR_ROW_COUNT attribute associated with an
      // INSERT statment represents the number of rows inserted.
      //
      return row_count != 0;
    }

    unsigned long long insert_statement::
    id ()
    {
#ifdef ODB_ORACLE_USE_64_BIT_ID
      return id_bind_.id.value_64;
#else
      return id_bind_.id.value_32;
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
      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               conn_.error_handle (),
                               1,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      conn_.error_handle ());

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

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
      sword r (OCIStmtExecute (conn_.handle (),
                               stmt_,
                               conn_.error_handle (),
                               1,
                               0,
                               0,
                               0,
                               OCI_DEFAULT));

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

      ub4 row_count (0);
      r = OCIAttrGet (stmt_,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      conn_.error_handle ());

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (conn_.error_handle (), r);

      return static_cast<unsigned long long> (row_count);
    }
  }
}
