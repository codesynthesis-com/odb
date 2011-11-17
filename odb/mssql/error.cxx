// file      : odb/mssql/error.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/mssql/odbc.hxx>
#include <odb/mssql/error.hxx>
//#include <odb/mssql/connection.hxx>
#include <odb/mssql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace mssql
  {
    static void
    translate_error (SQLHANDLE h, SQLSMALLINT htype, connection* /*conn*/)
    {
      char sqlstate[SQL_SQLSTATE_SIZE + 1];
      SQLINTEGER native_code; // Will be 0 if no natve code.
      char msg[512];          // Will be truncated if doesn't fit.
      SQLSMALLINT msg_size;

      database_exception e;

      for (SQLSMALLINT i (1);; ++i)
      {
        SQLRETURN r (SQLGetDiagRec (htype,
                                    h,
                                    i,
                                    (SQLCHAR*) sqlstate,
                                    &native_code,
                                    (SQLCHAR*) msg,
                                    sizeof (msg),
                                    &msg_size));

        if (r == SQL_NO_DATA)
          break;
        else if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO)
        {
          //@@ TODO: translate timeout, connection_lost exceptions
          //@@ TODO: if conn != 0, mark it as failed for certain errors

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
    translate_error (connection& /*c*/)
    {
      //@@ TODO enable (also header inclusion)
      // translate_error (c.handle (), SQL_HANDLE_DBC, &c);
    }

    LIBODB_MSSQL_EXPORT void
    translate_error (SQLHANDLE h, SQLSMALLINT htype)
    {
      translate_error (h, htype, 0);
    }
  }
}
