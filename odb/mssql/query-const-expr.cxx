// file      : odb/mssql/query-const-expr.cxx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/query.hxx>

namespace odb
{
  namespace mssql
  {
    // Sun CC cannot handle this in query.cxx.
    //
    const query_base query_base::true_expr (true);
  }
}
