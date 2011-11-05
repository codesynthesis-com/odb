// file      : odb/oracle/connection.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <string>

#include <oci.h>

#include <odb/oracle/database.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/transaction.hxx>
#include <odb/oracle/error.hxx>
#include <odb/oracle/exceptions.hxx>
#include <odb/oracle/auto-descriptor.hxx>
#include <odb/oracle/statement-cache.hxx>

using namespace std;

namespace odb
{
  namespace oracle
  {
    connection::
    connection (database_type& db)
        : odb::connection (db),
          db_ (db),
          statement_cache_ (new statement_cache_type (*this)),
          lob_buffer_ (0)
    {
      sword r (0);

      {
        OCIError* e (0);
        r = OCIHandleAlloc (db_.environment (),
                            reinterpret_cast<void**> (&e),
                            OCI_HTYPE_ERROR,
                            0,
                            0);

        if (r != OCI_SUCCESS)
          throw invalid_oci_handle ();

        error_.reset (e);
      }

      auto_handle<OCIAuthInfo> auth_info;
      {
        OCIAuthInfo* a (0);
        r = OCIHandleAlloc (db_.environment (),
                            reinterpret_cast<void**> (&a),
                            OCI_HTYPE_AUTHINFO,
                            0,
                            0);

        if (r != OCI_SUCCESS)
          throw invalid_oci_handle ();

        auth_info.reset (a);
      }

      r = OCIAttrSet (
        auth_info,
        OCI_HTYPE_AUTHINFO,
        reinterpret_cast<OraText*> (const_cast<char*> (db_.user ().c_str ())),
        static_cast <ub4> (db_.user ().size ()),
        OCI_ATTR_USERNAME,
        error_);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (error_, r);

      r = OCIAttrSet (
        auth_info,
        OCI_HTYPE_AUTHINFO,
        reinterpret_cast<OraText*> (
          const_cast<char*> (db_.password ().c_str ())),
        static_cast<ub4> (db_.password ().size ()),
        OCI_ATTR_PASSWORD,
        error_);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (error_, r);

      {
        OCISvcCtx* s (0);

        r = OCISessionGet (
          db_.environment (),
          error_,
          &s,
          auth_info,
          reinterpret_cast<OraText*> (const_cast<char*> (db_.db ().c_str ())),
          static_cast<ub4> (db_.db ().size ()),
          0,
          0,
          0,
          0,
          0,
          OCI_SESSGET_STMTCACHE);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (error_, r);

        handle_.reset (s, error_);
      }
    }

    connection::
    connection (database_type& db, OCISvcCtx* handle)
        : odb::connection (db),
          db_ (db),
          statement_cache_ (new statement_cache_type (*this)),
          lob_buffer_ (0)
    {
      sword r (0);

      {
        OCIError* e (0);
        r = OCIHandleAlloc (db_.environment (),
                            reinterpret_cast<void**> (&e),
                            OCI_HTYPE_ERROR,
                            0,
                            0);

        if (r != OCI_SUCCESS)
          throw invalid_oci_handle ();

        error_.reset (e);
      }

      handle_.reset (handle, error_);
    }

    connection::
    ~connection ()
    {
      // Deallocate prepared statements before we close the connection.
      //
      statement_cache_.reset ();
    }

    transaction_impl* connection::
    begin ()
    {
      return new transaction_impl (connection_ptr (inc_ref (this)));
    }

    unsigned long long connection::
    execute (const char* s, std::size_t n)
    {
      sword r (0);

      // OCI requires statement text to be NULL terminated.
      //
      string sql (s, n);

      auto_handle<OCIStmt> stmt;
      {
        OCIStmt* s (0);
        r = OCIStmtPrepare2 (handle_,
                             &s,
                             error_,
                             reinterpret_cast<const OraText*> (sql.c_str ()),
                             static_cast<ub4> (sql.size ()),
                             0,
                             0,
                             OCI_NTV_SYNTAX,
                             OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (error_, r);

        stmt.reset (s, OCI_STRLS_CACHE_DELETE, error_);
      }

      ub2 stmt_type;
      r = OCIAttrGet (stmt,
                      OCI_HTYPE_STMT,
                      &stmt_type,
                      0,
                      OCI_ATTR_STMT_TYPE,
                      error_);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (error_, r);

      ub4 row_count (0);

      if (stmt_type == OCI_STMT_SELECT)
      {
        // Do not prefetch any rows.
        //
        r = OCIStmtExecute (handle_, stmt, error_, 0, 0, 0, 0, OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (error_, r);

        // In order to succesfully execute a select statement, OCI/Oracle
        // requires that there be OCIDefine handles provided for all select
        // list columns. Since we are not interested in any data returned by
        // the select statement, all buffer pointers, indication variable
        // pointers, and data length pointers are specified as NULL (we still
        // specify a valid data type identifier as not doing so results in
        // undefined behaviour). This action results in truncation errors
        // being returned for all attempted row fetches. However, cursor
        // behaviour is normal, thus allowing us to return the row count for
        // a select statement.
        //
        for (ub4 i (1); ; ++i)
        {
          auto_descriptor<OCIParam> param;
          {
            OCIParam* p (0);
            r = OCIParamGet (stmt,
                             OCI_HTYPE_STMT,
                             error_,
                             reinterpret_cast<void**> (&p),
                             i);

            if (r == OCI_ERROR)
              break;

            param.reset (p);
          }

          ub2 data_type;
          r = OCIAttrGet (param,
                          OCI_DTYPE_PARAM,
                          &data_type,
                          0,
                          OCI_ATTR_DATA_TYPE,
                          error_);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (error_, r);

          // No need to keep track of the OCIDefine handles - these will
          // be deallocated with the statement.
          //
          OCIDefine* define (0);
          r = OCIDefineByPos (stmt,
                              &define,
                              error_,
                              i,
                              0,    // NULL value buffer pointer
                              0,    // zero length value buffer
                              data_type,
                              0,    // NULL indicator pointer
                              0,    // NULL length data pointer
                              0,    // NULL column level return code pointer
                              OCI_DEFAULT);

          if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
            translate_error (error_, r);
        }

        while (1)
        {
          r = OCIStmtFetch2 (stmt, error_, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);

          if (r == OCI_NO_DATA)
            break;
          else if (r == OCI_ERROR)
          {
            sb4 e;
            r = OCIErrorGet (error_, 1, 0, &e, 0, 0, OCI_HTYPE_ERROR);

            // ORA-01406 is returned if there is a truncation error. We expect
            // and ignore all truncation errors.
            //
            if (e != 1406)
              translate_error (error_, r);
          }
          else if (r ==  OCI_INVALID_HANDLE)
            translate_error (error_, r);
        }
      }
      else
      {
        // OCIStmtExecute requires a non-zero iters param for DML statements.
        //
        r = OCIStmtExecute (handle_, stmt, error_, 1, 0, 0, 0, OCI_DEFAULT);

        if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
          translate_error (error_, r);
      }

      r = OCIAttrGet (stmt,
                      OCI_HTYPE_STMT,
                      &row_count,
                      0,
                      OCI_ATTR_ROW_COUNT,
                      error_);

      if (r == OCI_ERROR || r == OCI_INVALID_HANDLE)
        translate_error (error_, r);

      return row_count;
    }
  }
}
