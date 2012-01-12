// file      : odb/mssql/query-const-expr.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
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
