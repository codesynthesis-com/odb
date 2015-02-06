// file      : odb/mssql/connection.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <string>

#include <odb/mssql/mssql.hxx>
#include <odb/mssql/database.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/transaction.hxx>
#include <odb/mssql/statement.hxx>
#include <odb/mssql/statement-cache.hxx>
#include <odb/mssql/error.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    static const long transaction_isolation_map[] =
    {
      SQL_TXN_READ_UNCOMMITTED,
      SQL_TXN_READ_COMMITTED,
      SQL_TXN_REPEATABLE_READ,
      SQL_TXN_SS_SNAPSHOT,
      SQL_TXN_SERIALIZABLE
    };

    connection::
    connection (database_type& db)
        : odb::connection (db),
          db_ (db),
          state_ (state_disconnected),
          statement_cache_ (new statement_cache_type (*this)),
          long_data_buffer_ (0)
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
      r = SQLSetConnectAttrA (handle_,
                              SQL_ATTR_AUTOCOMMIT,
                              (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
                              0);

      if (!SQL_SUCCEEDED (r))
        // Still use the handle version of translate_error since there
        // is no connection yet.
        //
        translate_error (r, handle_, SQL_HANDLE_DBC);

      // Enable Multiple Active Result Sets (MARS).
      //
      r = SQLSetConnectAttrA (handle_,
                              SQL_COPT_SS_MARS_ENABLED,
                              (SQLPOINTER) SQL_MARS_ENABLED_YES,
                              SQL_IS_UINTEGER);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, handle_, SQL_HANDLE_DBC);

      // Set transaction isolation level.
      //
      transaction_isolation ti (db_.transaction_isolation ());
      switch (ti)
      {
      case isolation_read_committed:
        {
          break; // SQL Server default.
        }
      case isolation_read_uncommitted:
      case isolation_repeatable_read:
      case isolation_serializable:
        {
          r = SQLSetConnectAttrA (handle_,
                                  SQL_ATTR_TXN_ISOLATION,
                                  (SQLPOINTER) transaction_isolation_map[ti],
                                  0);
          if (!SQL_SUCCEEDED (r))
            translate_error (r, handle_, SQL_HANDLE_DBC);
          break;
        }
      case isolation_snapshot:
        {
          r = SQLSetConnectAttrA (handle_,
                                  SQL_COPT_SS_TXN_ISOLATION,
                                  (SQLPOINTER) transaction_isolation_map[ti],
                                  SQL_IS_INTEGER);

          if (!SQL_SUCCEEDED (r))
            translate_error (r, handle_, SQL_HANDLE_DBC);
          break;
        }
      }

      // Connect.
      //
      {
        SQLSMALLINT out_conn_str_size;
        r = SQLDriverConnectA (handle_,
                               0, // Parent window handle.
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
          state_ (state_connected),
          statement_cache_ (new statement_cache_type (*this)),
          long_data_buffer_ (0)
    {
    }

    connection::
    ~connection ()
    {
      // Deallocate prepared statements before we close the connection.
      //
      recycle ();
      clear_prepared_map ();
      statement_cache_.reset ();
      direct_stmt_.reset ();

      if (state_ != state_disconnected)
        SQLDisconnect (handle_); // Ignore any errors.
    }

    transaction_impl* connection::
    begin ()
    {
      return new transaction_impl (connection_ptr (inc_ref (this)));
    }

    unsigned long long connection::
    execute (const char* s, std::size_t n)
    {
      {
        odb::tracer* t;
        if ((t = transaction_tracer ()) ||
            (t = tracer ()) ||
            (t = database ().tracer ()))
        {
          string str (s, n);
          t->execute (*this, str.c_str ());
        }
      }

      SQLRETURN r;

      // Allocate the statement if necessary.
      //
      if (direct_stmt_ == 0)
      {
        SQLHANDLE h;
        r = SQLAllocHandle (SQL_HANDLE_STMT, handle_, &h);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, *this);

        direct_stmt_.reset (h);

        // Disable escape sequences.
        //
        r = SQLSetStmtAttr (direct_stmt_,
                            SQL_ATTR_NOSCAN,
                            (SQLPOINTER) SQL_NOSCAN_OFF,
                            0);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, *this, direct_stmt_);

        // Disable data retrieval for SELECT statements.
        //
        r = SQLSetStmtAttr (direct_stmt_,
                            SQL_ATTR_RETRIEVE_DATA,
                            (SQLPOINTER) SQL_RD_OFF,
                            0);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, *this, direct_stmt_);
      }

      // Execute.
      //
      r = SQLExecDirectA (direct_stmt_, (SQLCHAR*) s, (SQLINTEGER) n);

      // SQL_NO_DATA indicates that a DML statement hasn't affected
      // any rows.
      //
      if (r == SQL_NO_DATA)
        return 0;

      if (!SQL_SUCCEEDED (r))
        translate_error (r, *this, direct_stmt_);

      // Get the number of affected/returned rows.
      //
      SQLLEN rows;

      // See if this is a select statement.
      //
      SQLSMALLINT cols;
      r = SQLNumResultCols (direct_stmt_, &cols);

      if (!SQL_SUCCEEDED (r))
        translate_error (r, *this, direct_stmt_);

      if (cols != 0)
      {
        for (rows = 0;; ++rows)
        {
          r = SQLFetch (direct_stmt_);

          if (r == SQL_NO_DATA)
            break;
          else if (!SQL_SUCCEEDED (r))
            translate_error (r, *this, direct_stmt_);
        }

        r = SQLCloseCursor (direct_stmt_);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, *this, direct_stmt_);
      }
      else
      {
        r = SQLRowCount (direct_stmt_, &rows);

        if (!SQL_SUCCEEDED (r))
          translate_error (r, *this, direct_stmt_);

        // -1 means the row count is not available. In particular, the
        // Native Client ODBC driver returns this value for DDL statements.
        //
        if (rows == -1)
          rows = 0;
      }

      return static_cast<unsigned long long> (rows);
    }
  }
}
