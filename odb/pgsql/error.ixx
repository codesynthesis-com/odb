// file      : odb/pgsql/error.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace pgsql
  {
    inline bool
    is_good_result (PGresult* r, ExecStatusType* s)
    {
      if (r != 0)
      {
        ExecStatusType status (PQresultStatus (r));

        if (s != 0)
          *s = status;

        return
          status != PGRES_BAD_RESPONSE
          && status != PGRES_NONFATAL_ERROR
          && status != PGRES_FATAL_ERROR
#ifdef LIBPQ_HAS_PIPELINING
          && status != PGRES_PIPELINE_ABORTED
#endif
          ;
      }

      return false;
    }
  }
}
