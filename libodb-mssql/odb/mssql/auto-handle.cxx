// file      : odb/mssql/auto-handle.cxx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/mssql.hxx>
#include <odb/mssql/auto-handle.hxx>

namespace odb
{
  namespace mssql
  {
    void
    free_handle (SQLHANDLE h, SQLSMALLINT htype)
    {
      SQLFreeHandle (htype, h);
    }
  }
}
