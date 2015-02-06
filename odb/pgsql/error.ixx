// file      : odb/pgsql/error.ixx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace pgsql
  {
    bool
    inline is_good_result (PGresult* r, ExecStatusType* s)
    {
      if (r != 0)
      {
        ExecStatusType status (PQresultStatus (r));

        if (s != 0)
          *s = status;

        return
          status != PGRES_BAD_RESPONSE &&
          status != PGRES_NONFATAL_ERROR &&
          status != PGRES_FATAL_ERROR;
      }

      return false;
    }
  }
}
