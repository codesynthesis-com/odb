// file      : odb/oracle/result.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_RESULT_HXX
#define ODB_ORACLE_RESULT_HXX

#include <odb/pre.hxx>

#include <odb/traits.hxx>
#include <odb/result.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T, class_kind kind>
    class result_impl;
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_RESULT_HXX

// Include result specializations so that the user code only needs
// to include this header.
//

#include <odb/oracle/object-result.hxx>
#include <odb/oracle/view-result.hxx>
