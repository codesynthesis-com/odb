// file      : odb/pgsql/errors.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <string>
#include <cassert>

#include <odb/pgsql/error.hxx>
#include <odb/pgsql/exceptions.hxx>

using namespace std;

namespace odb
{
  namespace pgsql
  {
    void
    translate_error (connection& c)
    {
      PGconn* h (0); // @@ c.handle ();

      assert (CONNECTION_BAD == PQstatus (h));

      if (const char* m = PQerrorMessage (h))
        throw database_exception (m);
      else
        throw database_exception ("unknown error");
    }

    void
    translate_error (connection& c, PGresult* r)
    {
      PGconn* h (0); // @@ c.handle ();

      if (!r)
      {
        if (CONNECTION_BAD == PQstatus (h))
          throw connection_lost ();
        else
          throw bad_alloc ();
      }

      switch (PQresultStatus (r))
      {
      case PGRES_BAD_RESPONSE:
        {
          if (const char* m = PQresultErrorMessage (r))
            throw database_exception (m);
          else
            throw database_exception ("bad server response");
        }

      case PGRES_FATAL_ERROR:
        {
          // PG_DIAG_SQLSTATE field is always present.
          //
          string s (PQresultErrorField (r, PG_DIAG_SQLSTATE));

          // Deadlock detected.
          //
          if ("40P01" == s)
            throw deadlock ();

          else if (CONNECTION_BAD == PQstatus (h))
            throw connection_lost ();

          else
            throw database_exception (s, PQresultErrorMessage (r));
        }

      default:
        assert (0);
        break;
      };
    }
  }
}
