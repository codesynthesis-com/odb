// file      : odb/oracle/connection.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <oci.h>

#include <string>

#include <odb/oracle/database.hxx>
#include <odb/oracle/connection.hxx>
#include <odb/oracle/transaction.hxx>
#include <odb/oracle/statement.hxx>
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
          failed_ (false),
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
          failed_ (false),
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
      recycle ();
      clear_prepared_map ();
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
      generic_statement st (*this, string (s, n));
      return st.execute ();
    }
  }
}
