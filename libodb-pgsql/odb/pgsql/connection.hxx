// file      : odb/pgsql/connection.hxx
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
    class connection_factory;

    class connection;
    typedef details::shared_ptr<connection> connection_ptr;

    class LIBODB_PGSQL_EXPORT connection: public odb::connection
    {
    public:
      typedef pgsql::statement_cache statement_cache_type;
      typedef pgsql::database database_type;

      virtual
      ~connection ();

      connection (connection_factory&);
      connection (connection_factory&, PGconn* handle);

      database_type&
      database ();

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

      // Server version as returned by PQserverVersion(), for example, 90200
      // (9.2.0), 90201 (9.2.1), 100000 (10.0), 110001 (11.1).
      //
      int
      server_version () const;

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
      auto_handle<PGconn> handle_;
      bool failed_;

      // Keep statement_cache_ after handle_ so that it is destroyed before
      // the connection is closed.
      //
      details::unique_ptr<statement_cache_type> statement_cache_;
    };

    class LIBODB_PGSQL_EXPORT connection_factory:
      public odb::connection_factory
    {
    public:
      typedef pgsql::database database_type;

      virtual void
      database (database_type&);

      database_type&
      database () {return *db_;}

      virtual connection_ptr
      connect () = 0;

      virtual
      ~connection_factory ();

      connection_factory (): db_ (0) {}

      // Needed to break the circular connection_factory-database dependency
      // (odb::connection_factory has the odb::database member).
      //
    protected:
      database_type* db_;
    };
  }
}

#include <odb/pgsql/connection.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_CONNECTION_HXX
