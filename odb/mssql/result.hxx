// file      : odb/mssql/result.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_RESULT_HXX
#define ODB_MSSQL_RESULT_HXX

#include <odb/pre.hxx>

#include <odb/traits.hxx>
#include <odb/result.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_RESULT_HXX

// Include result specializations so that the user code only needs
// to include this header.
//

#include <odb/mssql/object-result.hxx>
#include <odb/mssql/view-result.hxx>
