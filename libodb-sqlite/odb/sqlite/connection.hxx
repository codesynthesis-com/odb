// file      : odb/sqlite/connection.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_CONNECTION_HXX
#define ODB_SQLITE_CONNECTION_HXX

#include <odb/pre.hxx>

#include <sqlite3.h>

#include <odb/statement.hxx>
#include <odb/connection.hxx>

#include <odb/details/mutex.hxx>
#include <odb/details/condition.hxx>
#include <odb/details/shared-ptr.hxx>
#include <odb/details/unique-ptr.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/query.hxx>
#include <odb/sqlite/tracer.hxx>
#include <odb/sqlite/transaction-impl.hxx>
#include <odb/sqlite/auto-handle.hxx>

#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    class statement_cache;
    class generic_statement;
    class connection_factory;
    class attached_connection_factory;

    class connection;
    typedef details::shared_ptr<connection> connection_ptr;

    // SQLite "active object", i.e., an object that needs to be
    // "cleared" before the transaction can be committed and the
    // connection released. These form a doubly-linked list.
    //
    class LIBODB_SQLITE_EXPORT active_object
    {
    public:
      // This function may remove the object from the list since it may no
      // longer be "active".
      //
      virtual void
      clear () = 0;

    protected:
      active_object (connection& c): prev_ (0), next_ (this), conn_ (c) {}

      void
      list_add ();

      void
      list_remove ();

    protected:
      friend class connection;

      // prev_ == 0 means we are the first element.
      // next_ == 0 means we are the last element.
      // next_ == this means we are not on the list (prev_ should be 0).
      //
      active_object* prev_;
      active_object* next_;

      connection& conn_;
    };

    class LIBODB_SQLITE_EXPORT connection: public odb::connection
    {
    public:
      typedef sqlite::statement_cache statement_cache_type;
      typedef sqlite::database database_type;

      // Translate the database schema in the statement text (used to
      // implement attached databases). If the result is empty, then no
      // translation is required and the original text should be used as is.
      //
      typedef void (statement_translator) (std::string& result,
                                           const char* text,
                                           std::size_t text_size,
                                           connection&);
      virtual
      ~connection ();

      connection (connection_factory&,
                  int extra_flags = 0,
                  statement_translator* = 0);

      connection (connection_factory&,
                  sqlite3* handle,
                  statement_translator* = 0);

      // Create an attached connection (see the attached database constructor
      // for details).
      //
      connection (attached_connection_factory&, statement_translator*);

      database_type&
      database ();

      // Return the main connection of an attached connection. If this
      // connection is main, return itself.
      //
      connection&
      main_connection ();

      static connection_ptr
      main_connection (const connection_ptr&);

    public:
      virtual transaction_impl*
      begin ();

      transaction_impl*
      begin_immediate ();

      transaction_impl*
      begin_exclusive ();

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
      prepare_query (const char* name, const sqlite::query_base&);

      template <typename T>
      prepared_query<T>
      prepare_query (const char* name, const odb::query_base&);

      // SQL statement tracing.
      //
    public:
      typedef sqlite::tracer tracer_type;

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
      sqlite3*
      handle ();

      statement_cache_type&
      statement_cache ()
      {
        return *statement_cache_;
      }

      // Wait for the locks to be released via unlock notification. Can
      // be called after getting SQLITE_LOCKED_SHAREDCACHE.
      //
      void
      wait ();

    public:
      // Reset active statements. Also invalidates query results by first
      // calling invalidate_results().
      //
      void
      clear ();

    public:
      // Note: only available on main connection.
      //
      generic_statement&
      begin_statement ();

      generic_statement&
      begin_immediate_statement ();

      generic_statement&
      begin_exclusive_statement ();

      generic_statement&
      commit_statement ();

      generic_statement&
      rollback_statement ();

    protected:
      friend class attached_connection_factory;

      connection_factory&
      factory ();

    private:
      connection (const connection&);
      connection& operator= (const connection&);

    private:
      void
      init ();

    private:
      // Note that we use NULL handle as an indication of an attached
      // connection.
      //
      auto_handle<sqlite3> handle_;

      statement_translator* statement_translator_;

      // Keep statement_cache_ after handle_ so that it is destroyed before
      // the connection is closed.
      //
      details::unique_ptr<statement_cache_type> statement_cache_;

      // Note: using odb::statement in order to break the connection-statement
      // dependency cycle.
      //
      details::shared_ptr<odb::statement> begin_;
      details::shared_ptr<odb::statement> begin_immediate_;
      details::shared_ptr<odb::statement> begin_exclusive_;
      details::shared_ptr<odb::statement> commit_;
      details::shared_ptr<odb::statement> rollback_;

      // Unlock notification machinery.
      //
    private:
      bool unlocked_;
      details::mutex unlock_mutex_;
      details::condition unlock_cond_;

      friend void
      connection_unlock_callback (void**, int);

    private:
      friend class statement;        // statement_translator_
      friend class transaction_impl; // invalidate_results()

      // Linked list of active objects currently associated
      // with this connection.
      //
    private:
      friend class active_object;
      active_object* active_objects_;
    };

    class LIBODB_SQLITE_EXPORT connection_factory:
      public odb::connection_factory
    {
    public:
      typedef sqlite::database database_type;

      virtual void
      database (database_type&);

      database_type&
      database () {return *db_;}

      virtual connection_ptr
      connect () = 0;

      virtual
      ~connection_factory ();

      connection_factory (): db_ (0) {}

      // Attach/detach additional databases. Connection is one of the main
      // connections created by this factory. Note: not called for "main" and
      // "temp" schemas.
      //
      // The default implementations simply execute the ATTACH DATABASE and
      // DETACH DATABASE SQLite statements.
      //
      virtual void
      attach_database (const connection_ptr&,
                       const std::string& name,
                       const std::string& schema);

      virtual void
      detach_database (const connection_ptr&, const std::string& schema);

      // Needed to break the circular connection_factory-database dependency
      // (odb::connection_factory has the odb::database member).
      //
    protected:
      database_type* db_;
    };

    // The call to database() should cause ATTACH DATABASE (or otherwise make
    // sure the database is attached). Destruction of the factory should cause
    // DETACH DATABASE (or otherwise notice that this factory no longer needs
    // the database attached).
    //
    // Note that attached_connection_factory is an active object that
    // registers itself with the main connection in order to get notified on
    // transaction finalization.
    //
    class LIBODB_SQLITE_EXPORT attached_connection_factory:
      public connection_factory,
      public active_object
    {
    public:
      explicit
      attached_connection_factory (const connection_ptr& main)
          : active_object (*main), main_connection_ (main) {}

      virtual void
      detach () = 0;

    protected:
      friend class database;
      friend class connection;
      friend class transaction_impl;

      connection_factory&
      main_factory ();

      // Note that this essentially establishes a "framework" for all the
      // attached connection factory implementations: they hold a counted
      // reference to the main connection and they maintain a single shared
      // attached connection.
      //
      connection_ptr main_connection_;
      connection_ptr attached_connection_;
    };
  }
}

#include <odb/sqlite/connection.ixx>

#include <odb/post.hxx>

#endif // ODB_SQLITE_CONNECTION_HXX
