// file      : odb/pgsql/connection.hxx
// copyright : Copyright (c) 2005-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_CONNECTION_HXX
#define ODB_PGSQL_CONNECTION_HXX

#include <odb/pre.hxx>

#include <odb/connection.hxx>

#include <odb/details/shared-ptr.hxx>
#include <odb/details/unique-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/query.hxx>
#include <odb/pgsql/tracer.hxx>
#include <odb/pgsql/transaction-impl.hxx>
#include <odb/pgsql/auto-handle.hxx>
#include <odb/pgsql/pgsql-fwd.hxx> // PGconn

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class statement_cache;

    class connection;
    typedef details::shared_ptr<connection> connection_ptr;

    class LIBODB_PGSQL_EXPORT connection: public odb::connection
    {
    public:
      typedef pgsql::statement_cache statement_cache_type;
      typedef pgsql::database database_type;

      virtual
      ~connection ();

      connection (database_type&);
      connection (database_type&, PGconn* handle);

      database_type&
      database ()
      {
        return db_;
      }

    public:
      virtual transaction_impl*
      begin ();

    public:
      using odb::connection::execute;

      virtual unsigned long long
      execute (const char* statement, std::size_t length);

      // Query preparation.
      //
    public:
      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const char*);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const std::string&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const pgsql::query_base&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const odb::query_base&);

      // SQL statement tracing.
      //
    public:
      typedef pgsql::tracer tracer_type;

      void
      tracer (tracer_type& t)
      {
        odb::connection::tracer (t);
      }

      void
      tracer (tracer_type* t)
      {
        odb::connection::tracer (t);
      }

      using odb::connection::tracer;

    public:
      bool
      failed () const
      {
        return failed_;
      }

      void
      mark_failed ()
      {
        failed_ = true;
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
      void
      init ();

    private:
      friend class transaction_impl; // invalidate_results()

    private:
      // Needed to break the circular connection-database dependency
      // (odb::connection has the odb::database member).
      //
      database_type& db_;

      auto_handle<PGconn> handle_;
      bool failed_;

      // Keep statement_cache_ after handle_ so that it is destroyed before
      // the connection is closed.
      //
      details::unique_ptr<statement_cache_type> statement_cache_;
    };
  }
}

#include <odb/pgsql/connection.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_CONNECTION_HXX
