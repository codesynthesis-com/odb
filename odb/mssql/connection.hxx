// file      : odb/mssql/connection.hxx
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_CONNECTION_HXX
#define ODB_MSSQL_CONNECTION_HXX

#include <odb/pre.hxx>

#include <odb/connection.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/shared-ptr.hxx>
#include <odb/details/unique-ptr.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/query.hxx>
#include <odb/mssql/tracer.hxx>
#include <odb/mssql/transaction-impl.hxx>
#include <odb/mssql/auto-handle.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class statement_cache;
    class connection_factory;

    class connection;
    typedef details::shared_ptr<connection> connection_ptr;

    class LIBODB_MSSQL_EXPORT connection: public odb::connection
    {
    public:
      typedef mssql::statement_cache statement_cache_type;
      typedef mssql::database database_type;

      virtual
      ~connection ();

      connection (connection_factory&);
      connection (connection_factory&, SQLHDBC handle);

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
      prepare_query (const char* name, const mssql::query_base&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const odb::query_base&);

      // SQL statement tracing.
      //
    public:
      typedef mssql::tracer tracer_type;

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
        return state_ == state_failed;
      }

      void
      mark_failed ()
      {
        state_ = state_failed;
      }

    public:
      SQLHDBC
      handle ()
      {
        return handle_;
      }

      statement_cache_type&
      statement_cache ()
      {
        return *statement_cache_;
      }

      details::buffer&
      long_data_buffer ()
      {
        return long_data_buffer_;
      }

    private:
      connection (const connection&);
      connection& operator= (const connection&);

    private:
      friend class transaction_impl; // invalidate_results()

    private:
      auto_handle<SQL_HANDLE_DBC> handle_;

      enum
      {
        state_disconnected,
        state_connected,
        state_failed
      } state_;

      // Statement handle for direct execution.
      //
      auto_handle<SQL_HANDLE_STMT> direct_stmt_;
      details::unique_ptr<statement_cache_type> statement_cache_;
      details::buffer long_data_buffer_;
    };

    class LIBODB_MSSQL_EXPORT connection_factory:
      public odb::connection_factory
    {
    public:
      typedef mssql::database database_type;

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

#include <odb/mssql/connection.ixx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_CONNECTION_HXX
