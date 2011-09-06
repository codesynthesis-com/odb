// file      : odb/pgsql/statements-base.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_STATEMENTS_BASE_HXX
#define ODB_PGSQL_STATEMENTS_BASE_HXX

#include <odb/pre.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx> // connection

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class LIBODB_PGSQL_EXPORT statements_base: public details::shared_base
    {
    public:
      typedef pgsql::connection connection_type;

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

#endif // ODB_PGSQL_STATEMENTS_BASE_HXX
