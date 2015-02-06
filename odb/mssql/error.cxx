// file      : odb/mssql/error.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <string>
#include <cstring> // std::strlen

#include <odb/mssql/mssql.hxx>
#include <odb/mssql/error.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    static void
    translate_error (SQLRETURN r,
                     SQLHANDLE h,
                     SQLSMALLINT htype,
                     connection* conn,
                     bool end_tran,
                     size_t pos,
                     multiple_exceptions* mex)
    {
      // First see if we have one of the errors indicated via the
      // return error code.
      //
      switch (r)
      {
      case SQL_STILL_EXECUTING:
        {
          throw database_exception (0, "?????", "statement still executing");
          break;
        }
      case SQL_NEED_DATA:
      case SQL_NO_DATA:
#if ODBCVER >= 0x0380
      case SQL_PARAM_DATA_AVAILABLE:
#endif
        {
          throw database_exception (
            0, "?????", "unhandled SQL_*_DATA condition");
          break;
        }
      case SQL_INVALID_HANDLE:
        {
          throw database_exception (0, "?????", "invalid handle");
          break;
        }
      }

      // Otherwise the diagnostics is stored in the handle.
      //
      char sqlstate[SQL_SQLSTATE_SIZE + 1];
      SQLINTEGER native_code; // Will be 0 if no natve code.
      char msg[512];          // Will be truncated if doesn't fit.
      SQLSMALLINT msg_size;

      // We need to translate certain sqlstate codes to special exceptions,
      // such as deadlock, timeout, etc. The problem is we can have multiple
      // records potentially with different sqlstate codes. If we have both,
      // say, a deadlock code and some other code, then we should probably
      // throw database_exception, which is more severe. To implement this
      // we are going to pre-scan the records looking for the codes we are
      // interested in. If in the process we see any other code, then we
      // stop and go ahead to prepare and throw database_exception.
      //
      enum code
      {
        code_none,
        code_deadlock,
        code_timeout,
        code_connection_lost
      };

      code c (code_none);

      for (SQLSMALLINT i (1);; ++i)
      {
        r = SQLGetDiagRecA (htype,
                            h,
                            i,
                            (SQLCHAR*) sqlstate,
                            &native_code,
                            0,
                            0,
                            &msg_size);

        if (r == SQL_NO_DATA)
          break;
        else if (SQL_SUCCEEDED (r))
        {
          code nc;
          string s (sqlstate);

          if (s == "40001")      // Serialization failure (native code 1205).
            nc = code_deadlock;
          else if (s == "HYT00") // Timeout expired.
            nc = code_timeout;
          else if (s == "HYT01") // Connection timeout expired.
          {
            nc = code_timeout;

            if (conn != 0)
              conn->mark_failed ();
          }
          else if (s == "08S01") // Link failure.
          {
            nc = code_connection_lost;

            if (conn != 0)
              conn->mark_failed ();
          }
          else if (s == "01000") // General warning.
            continue;
          else
          {
            c = code_none;
            break;
          }

          // If a call to SQLEndTran() fails, then the connection is
          // put into the so called "suspended state" and should be
          // disconnected unless we know the transaction was rolled
          // back. See SQLEndTran() documentation for details.
          //
          if (end_tran &&
              s != "25S03" && // Transaction is rolled back.
              s != "40001" && // Serialization failure.
              s != "40002" && // Integrity constraint.
              s != "HYC00")   // Optional feature not implemented.
            conn->mark_failed ();

          if (c != code_none && c != nc)
          {
            // Several different codes.
            //
            c = code_none;
            break;
          }

          c = nc;
        }
        else
        {
          c = code_none;
          break;
        }
      }

      switch (c)
      {
      case code_deadlock:
        throw deadlock ();
      case code_timeout:
        throw timeout ();
      case code_connection_lost:
        throw connection_lost ();
      case code_none:
        break;
      }

      // Some other error code. Prepare database_exception.
      //
      database_exception e;

      for (SQLSMALLINT i (1);; ++i)
      {
        // If this is for a batch, filter out based on row association.
        // Here we only ignore records that have the associated row
        // number and this number doesn't match ours. In particular,
        // this means that all the un-associated records which will be
        // duplicated for all the failed rows, which seems like the
        // correct thing to do.
        //
        if (mex != 0)
        {
          SQLLEN n;
          r = SQLGetDiagField (htype,
                               h,
                               i,
                               SQL_DIAG_ROW_NUMBER,
                               &n,
                               0,
                               0);

          if (r == SQL_NO_DATA)
              break;
          else if (SQL_SUCCEEDED (r) &&
                   n != SQL_NO_ROW_NUMBER &&
                   n != SQL_ROW_NUMBER_UNKNOWN &&
                   n != static_cast<SQLLEN> (pos + 1)) // 1-based
            continue;
        }

        r = SQLGetDiagRecA (htype,
                            h,
                            i,
                            (SQLCHAR*) sqlstate,
                            &native_code,
                            (SQLCHAR*) msg,
                            sizeof (msg),
                            &msg_size);

        if (r == SQL_NO_DATA)
          break;
        else if (SQL_SUCCEEDED (r))
        {
          if (conn != 0)
          {
            string s (sqlstate);

            if (s == "08S01" || // Link failure.
                s == "HYT01" || // Connection timeout.
                (end_tran &&
                 s != "25S03" &&
                 s != "40001" &&
                 s != "40002" &&
                 s != "HYC00"))
              conn->mark_failed ();
          }

          // Get rid of a trailing newline if there is one.
          //
          size_t n (strlen (msg));
          if (n != 0 && msg[n - 1] == '\n')
            msg[n - 1] = '\0';

          e.append (native_code, sqlstate, msg);
        }
        else
          e.append (0, "?????", "unable to extract information for this "
                    "diagnostic record");
      }

      if (e.size () == 0)
        e.append (0, "?????", "no diagnostic record (using wrong handle?)");

      if (mex == 0)
        throw e;
      else
        // It could be that some of these errors are fatal. I guess we
        // will just have to learn from experience which ones are. The
        // client code can always treat specific error codes as fatal.
        //
        mex->insert (pos, e);
    }

    void
    translate_error (SQLRETURN r, connection& c, bool end_tran)
    {
      translate_error (r, c.handle (), SQL_HANDLE_DBC, &c, end_tran, 0, 0);
    }

    void
    translate_error (SQLRETURN r,
                     connection& c,
                     const auto_handle<SQL_HANDLE_STMT>& h,
                     size_t pos,
                     multiple_exceptions* mex)
    {
      translate_error (r, h, SQL_HANDLE_STMT, &c, false, pos, mex);
    }

    void
    translate_error (SQLRETURN r, SQLHANDLE h, SQLSMALLINT htype)
    {
      translate_error (r, h, htype, 0, false, 0, 0);
    }
  }
}
