// file      : odb/oracle/result.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_RESULT_HXX
#define ODB_ORACLE_RESULT_HXX

#include <odb/pre.hxx>

#include <odb/traits.hxx>
#include <odb/result.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_RESULT_HXX

// Include result specializations so that the user code only needs
// to include this header.
//

#include <odb/oracle/object-result.hxx>
#include <odb/oracle/view-result.hxx>
