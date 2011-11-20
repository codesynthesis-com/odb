// file      : odb/mssql/connection.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

//@@ disabled functionality

//#include <string>

#include <odb/mssql/mssql.hxx>
#include <odb/mssql/database.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/transaction.hxx>
//#include <odb/mssql/statement.hxx>
//#include <odb/mssql/statement-cache.hxx>
#include <odb/mssql/error.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    connection::
    connection (database_type& db)
        : odb::connection (db),
          db_ (db),
          state_ (state_disconnected)
          // statement_cache_ (new statement_cache_type (*this))
    {
      SQLRETURN r;

      // Allocate the connection handle.
      //
      {
        SQLHANDLE h;
        r = SQLAllocHandle (SQL_HANDLE_DBC, db_.environment (), &h);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, db_.environment (), SQL_HANDLE_ENV);

        handle_.reset (h);
      }

      // Set the manual commit mode.
      //
      r = SQLSetConnectAttr (handle_,
                             SQL_ATTR_AUTOCOMMIT,
                             SQL_AUTOCOMMIT_OFF,
                             SQL_IS_UINTEGER);

      if (!SQL_SUCCEEDED (r))
        // Still use the handle version of translate_error since there
        // is no connection yet.
        //
        translate_error (r, handle_, SQL_HANDLE_DBC);

      // Connect.
      //
      {
        SQLSMALLINT out_conn_str_size;
        r = SQLDriverConnect (handle_,
                              0, // Parent windows handle.
                              (SQLCHAR*) db_.connect_string ().c_str (),
                              SQL_NTS,
                              0, // Output connection string buffer.
                              0, // Size of output connection string buffer.
                              &out_conn_str_size,
                              SQL_DRIVER_NOPROMPT);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, handle_, SQL_HANDLE_DBC);

        state_ = state_connected;
      }

      // If an exception is thrown after this line, we will not disconnect
      // the connection.
      //
    }

    connection::
    connection (database_type& db, SQLHDBC handle)
        : odb::connection (db),
          db_ (db),
          handle_ (handle),
          state_ (state_connected)
          // statement_cache_ (new statement_cache_type (*this))
    {
    }

    connection::
    ~connection ()
    {
      // Deallocate prepared statements before we close the connection.
      //
      //statement_cache_.reset ();

      if (state_ != state_disconnected)
        SQLDisconnect (handle_); // Ignore any errors.
    }

    transaction_impl* connection::
    begin ()
    {
      return new transaction_impl (connection_ptr (inc_ref (this)));
    }

    unsigned long long connection::
    execute (const char* /*s*/, std::size_t /*n*/)
    {
      //@@
      //generic_statement st (*this, string (s, n));
      //return st.execute ();
      return 0;
    }
  }
}
