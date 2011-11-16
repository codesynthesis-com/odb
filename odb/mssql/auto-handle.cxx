// file      : odb/mssql/auto-handle.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/odbc.hxx>
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
