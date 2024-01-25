// file      : odb/mysql/query-const-expr.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/mysql/query.hxx>

namespace odb
{
  namespace mysql
  {
    // Sun CC cannot handle this in query.cxx.
    //
    const query_base query_base::true_expr (true);
  }
}
