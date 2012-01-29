// file      : odb/mssql/statements-base.hxx
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_STATEMENTS_BASE_HXX
#define ODB_MSSQL_STATEMENTS_BASE_HXX

#include <odb/pre.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx> // connection

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class LIBODB_MSSQL_EXPORT statements_base: public details::shared_base
    {
    public:
      typedef mssql::connection connection_type;

      connection_type&
      connection ()
      {
        return conn_;
      }

    public:
      virtual
      ~statements_base ();

    protected:
      statements_base (connection_type& conn)
        : conn_ (conn)
      {
      }

    protected:
      connection_type& conn_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_STATEMENTS_BASE_HXX
