// file      : odb/oracle/query-const-expr.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/oracle/query.hxx>

namespace odb
{
  namespace oracle
  {
    // Sun CC cannot handle this in query.cxx.
    //
    const query_base query_base::true_expr (true);
  }
}
