// file      : odb/oracle/connection.hxx
// copyright : Copyright (c) 2005-2018 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_CONNECTION_HXX
#define ODB_ORACLE_CONNECTION_HXX

#include <odb/pre.hxx>

#include <odb/connection.hxx>

#include <odb/details/buffer.hxx>
#include <odb/details/shared-ptr.hxx>
#include <odb/details/unique-ptr.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/query.hxx>
#include <odb/oracle/tracer.hxx>
#include <odb/oracle/transaction-impl.hxx>
#include <odb/oracle/auto-handle.hxx>
#include <odb/oracle/oracle-fwd.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class statement_cache;
    class connection_factory;

    class connection;
    typedef details::shared_ptr<connection> connection_ptr;

    class LIBODB_ORACLE_EXPORT connection: public odb::connection
    {
    public:
      typedef oracle::statement_cache statement_cache_type;
      typedef oracle::database database_type;

      virtual
      ~connection ();

      connection (connection_factory&);
      connection (connection_factory&, OCISvcCtx* handle);

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
      prepare_query (const char* name, const oracle::query_base&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const odb::query_base&);

      // SQL statement tracing.
      //
    public:
      typedef oracle::tracer tracer_type;

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
      OCISvcCtx*
      handle ()
      {
        return handle_;
      }

      OCIError*
      error_handle ()
      {
        return error_;
      }

      statement_cache_type&
      statement_cache ()
      {
        return *statement_cache_;
      }

      details::buffer&
      lob_buffer ()
      {
        return lob_buffer_;
      }

    private:
      connection (const connection&);
      connection& operator= (const connection&);

    private:
      friend class transaction_impl; // invalidate_results()

    private:
      // It is important that the error_ member is declared before the
      // handle_ member as handle_ depends on error_ during destruction.
      //
      auto_handle<OCIError> error_;

      auto_handle<OCISvcCtx> handle_;
      bool failed_;

      details::unique_ptr<statement_cache_type> statement_cache_;
      details::buffer lob_buffer_;
    };

    class LIBODB_ORACLE_EXPORT connection_factory:
      public odb::connection_factory
    {
    public:
      typedef oracle::database database_type;

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

#include <odb/oracle/connection.ixx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_CONNECTION_HXX
