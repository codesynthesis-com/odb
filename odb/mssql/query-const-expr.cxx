// file      : odb/mssql/query-const-expr.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/query.hxx>

namespace odb
{
  namespace mssql
  {
    // Sun CC cannot handle this in query.cxx.
    //
    const query query::true_expr (true);
  }
}
