// file      : odb/oracle/prepared-query.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_PREPARED_QUERY_HXX
#define ODB_ORACLE_PREPARED_QUERY_HXX

#include <odb/pre.hxx>

#include <odb/prepared-query.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/query.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    struct LIBODB_ORACLE_EXPORT prepared_query_impl: odb::prepared_query_impl
    {
      virtual
      ~prepared_query_impl ();

      prepared_query_impl (odb::connection& c): odb::prepared_query_impl (c) {}

      oracle::query_base query;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_PREPARED_QUERY_HXX
