// file      : odb/pgsql/connection.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_CONNECTION_HXX
#define ODB_PGSQL_CONNECTION_HXX

#include <odb/pre.hxx>

#include <vector>
#include <memory> // std::auto_ptr

#include <libpq-fe.h>

#include <odb/forward.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class statement;
    class statement_cache;

    class LIBODB_PGSQL_EXPORT connection: public details::shared_base
    {
    public:
      typedef pgsql::statement_cache statement_cache_type;
      typedef pgsql::database database_type;

      virtual
      ~connection ();

      connection (database_type&);

      database_type&
      database ()
      {
        return db_;
      }

    public:
      PGconn*
      handle ()
      {
        return handle_;
      }

      statement_cache_type&
      statement_cache ()
      {
        return *statement_cache_;
      }

    private:
      connection (const connection&);
      connection& operator= (const connection&);

    private:
      database_type& db_;

      PGconn* handle_;

      std::auto_ptr<statement_cache_type> statement_cache_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_CONNECTION_HXX
