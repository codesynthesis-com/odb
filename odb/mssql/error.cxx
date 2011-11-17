// file      : odb/mssql/error.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <string>

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
    translate_error (SQLHANDLE h, SQLSMALLINT htype, connection* conn)
    {
      SQLRETURN r;

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
        code_connection_lost,
      };

      code c (code_none);

      for (SQLSMALLINT i (1);; ++i)
      {
        r = SQLGetDiagRec (htype,
                           h,
                           i,
                           (SQLCHAR*) sqlstate,
                           &native_code,
                           0,
                           0,
                           &msg_size);

        string s (sqlstate);

        if (r == SQL_NO_DATA)
          break;
        else if (SQL_SUCCEEDED (r))
        {
          code nc;

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
         r = SQLGetDiagRec (htype,
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
                s == "HYT01")   // Connection timeout.
              conn->mark_failed ();
          }

          e.append (native_code, sqlstate, msg);
        }
        else
          e.append (0, "?????", "unable to extract information for this "
                    "diagnostic record");
      }

      if (e.size () == 0)
        e.append (0, "?????", "no diagnostic record (using wrong handle?)");

      throw e;
    }

    void
    translate_error (connection& c)
    {
      translate_error (c.handle (), SQL_HANDLE_DBC, &c);
    }

    void
    translate_error (connection& c, const auto_handle<SQL_HANDLE_STMT>& h)
    {
      translate_error (h, SQL_HANDLE_STMT, &c);
    }

    void
    translate_error (SQLHANDLE h, SQLSMALLINT htype)
    {
      translate_error (h, htype, 0);
    }
  }
}
