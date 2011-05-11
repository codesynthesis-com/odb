// file      : odb/pgsql/errors.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <string>
#include <cassert>

#include <odb/pgsql/error.hxx>
#include <odb/pgsql/exceptions.hxx>
#include <odb/pgsql/connection.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    void
    translate_connection_error (connection& c)
    {
      PGconn* h (c.handle ());

      assert (CONNECTION_BAD == PQstatus (h));

      if (const char* m = PQerrorMessage (h))
        throw database_exception (m);
      else
        throw database_exception ("unknown error");
    }

    void
    translate_result_error_ (connection& c,
                             ExecStatusType status = PGRES_EMPTY_QUERY,
                             const char* sqlstate = 0,
                             const char* error_message = 0)
    {
      PGconn* h (c.handle ());

      switch (status)
      {
      case PGRES_BAD_RESPONSE:
        {
          if (error_message != 0)
            throw database_exception (error_message);
          else
            throw database_exception ("bad server response");
        }

      case PGRES_FATAL_ERROR:
        {
          assert (sqlstate);
          assert (error_message);

          // Deadlock detected.
          //
          if (std::string ("40P01") == sqlstate)
            throw deadlock ();

          else if (CONNECTION_BAD == PQstatus (h))
            throw connection_lost ();

          else
            throw database_exception (sqlstate, error_message);
        }

      default:
        {
          if (CONNECTION_BAD == PQstatus (h))
            throw connection_lost ();
          else
            throw bad_alloc ();
        }
      };
    }

    void
    translate_result_error (connection& c,
                            PGresult* r,
                            ExecStatusType s,
                            bool clear_result)
    {
      if (!r)
        translate_result_error_ (c);
      else
      {
        const char* ss = PQresultErrorField (r, PG_DIAG_SQLSTATE);
        const char* m = PQresultErrorMessage (r);

        if (clear_result)
          PQclear (r);

        translate_result_error_ (c, s, ss, m);
      }
    }
  }
}
