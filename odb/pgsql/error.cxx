// file      : odb/pgsql/error.cxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
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
    translate_error (connection& c, PGresult* r)
    {
      if (!r)
      {
        if (CONNECTION_BAD == PQstatus (c.handle ()))
        {
          c.mark_failed ();
          throw connection_lost ();
        }
        else
          throw bad_alloc ();
      }

      string msg;
      {
        // Can be NULL in case of PGRES_BAD_RESPONSE.
        //
        const char* m (PQresultErrorMessage (r));
        msg = (m != 0 ? m : "bad server response");

        // Get rid of a trailing newline if there is one.
        //
        string::size_type n (msg.size ());
        if (n != 0 && msg[n - 1] == '\n')
          msg.resize (n - 1);
      }

      switch (PQresultStatus (r))
      {
      case PGRES_BAD_RESPONSE:
        {
          throw database_exception (msg);
        }
      case PGRES_FATAL_ERROR:
        {
          string ss;
          {
            const char* s (PQresultErrorField (r, PG_DIAG_SQLSTATE));
            ss = (s != 0 ? s : "?????");
          }

          // Deadlock detected.
          //
          if (ss == "40P01")
            throw deadlock ();
          else if (CONNECTION_BAD == PQstatus (c.handle ()))
          {
            c.mark_failed ();
            throw connection_lost ();
          }
          else
            throw database_exception (ss, msg);
        }
      default:
        assert (false);
        break;
      }
    }
  }
}
