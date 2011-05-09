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
      // typedef pgsql::statement_cache statement_cache_type;
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

      // statement_cache_type&
      // statement_cache ()
      // {
      //   return *statement_cache_;
      // }

    public:
      // statement*
      // active ()
      // {
      //   return active_;
      // }

      // void
      // active (statement* s)
      // {
      //   active_ = s;

      //   if (s == 0 && stmt_handles_.size () > 0)
      //     free_stmt_handles ();
      // }

      // // Cancel and clear the active statement, if any.
      // //
      // void
      // clear ()
      // {
      //   if (active_ != 0)
      //     clear_ ();
      // }

    public:
      // MYSQL_STMT*
      // alloc_stmt_handle ();

      // void
      // free_stmt_handle (MYSQL_STMT*);

    private:
      connection (const connection&);
      connection& operator= (const connection&);

    private:
      // void
      // free_stmt_handles ();

      // void
      // clear_ ();

    private:
      database_type& db_;

      PGconn* handle_;

      // statement* active_;
      // std::auto_ptr<statement_cache_type> statement_cache_;

      // typedef std::vector<MYSQL_STMT*> stmt_handles;
      // stmt_handles stmt_handles_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_CONNECTION_HXX
