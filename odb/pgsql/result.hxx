// file      : odb/pgsql/result.hxx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_RESULT_HXX
#define ODB_PGSQL_RESULT_HXX

#include <odb/pre.hxx>

#include <odb/traits.hxx>
#include <odb/result.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_RESULT_HXX

// Include result specializations so that the user code only needs
// to include this header.
//

#include <odb/pgsql/object-result.hxx>
#include <odb/pgsql/view-result.hxx>
