// file      : odb/pgsql/error.cxx
// copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
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

      const char* error_message (PQresultErrorMessage (r));

      switch (PQresultStatus (r))
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
          const char* ss (PQresultErrorField (r, PG_DIAG_SQLSTATE));

          assert (ss);
          assert (error_message);

          // Deadlock detected.
          //
          if (std::string ("40P01") == ss)
            throw deadlock ();

          else if (CONNECTION_BAD == PQstatus (c.handle ()))
          {
            c.mark_failed ();
            throw connection_lost ();
          }
          else
            throw database_exception (ss, error_message);
        }
      default:
        assert (0);
        break;
      }
    }
  }
}
