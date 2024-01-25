// file      : odb/pgsql/auto-handle.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <libpq-fe.h>

#include <odb/pgsql/auto-handle.hxx>

namespace odb
{
  namespace pgsql
  {
    void handle_traits<PGconn>::
    release (PGconn* h)
    {
      PQfinish (h);
    }

    void handle_traits<PGresult>::
    release (PGresult* h)
    {
      PQclear (h);
    }
  }
}
