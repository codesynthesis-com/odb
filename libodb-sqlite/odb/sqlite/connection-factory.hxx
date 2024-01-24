// file      : odb/sqlite/connection-factory.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_CONNECTION_FACTORY_HXX
#define ODB_SQLITE_CONNECTION_FACTORY_HXX

#include <odb/pre.hxx>

#include <vector>
#include <cstddef> // std::size_t
#include <cassert>

#include <odb/details/mutex.hxx>
#include <odb/details/condition.hxx>
#include <odb/details/shared-ptr.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/details/export.hxx>

namespace odb
{
  namespace sqlite
  {
    // Share a single connection in a guaranteed serial database access.
    //
    // For example, a single-threaded application that executes all the
    // operations via the database instance without messing with multiple
    // connections/transactions would qualify.
    //
    class LIBODB_SQLITE_EXPORT serial_connection_factory:
      public connection_factory
    {
    public:
      serial_connection_factory () {}

      virtual connection_ptr
      connect ();

      virtual void
      database (database_type&);

      virtual
      ~serial_connection_factory ();

    private:
      serial_connection_factory (const serial_connection_factory&);
      serial_connection_factory& operator= (const serial_connection_factory&);

    protected:
      // This function is called when the factory needs to create the
      // connection.
      //
      virtual connection_ptr
      create ();

      connection_ptr connection_;
    };

    // Share a single connection potentially between multiple threads.
    //
    class LIBODB_SQLITE_EXPORT single_connection_factory:
      public connection_factory
    {
    public:
      single_connection_factory () {}

      virtual connection_ptr
      connect ();

      virtual void
      database (database_type&);

      virtual
      ~single_connection_factory ();

    private:
      single_connection_factory (const single_connection_factory&);
      single_connection_factory& operator= (const single_connection_factory&);

    protected:
      class LIBODB_SQLITE_EXPORT single_connection: public connection
      {
      public:
        single_connection (single_connection_factory&, int extra_flags = 0);
        single_connection (single_connection_factory&, sqlite3*);

      private:
        static bool
        zero_counter (void*);

      private:
        friend class single_connection_factory;
        shared_base::refcount_callback cb_;
      };

      friend class single_connection;

      typedef details::shared_ptr<single_connection> single_connection_ptr;

      // This function is called when the factory needs to create the
      // connection.
      //
      virtual single_connection_ptr
      create ();

    protected:
      // Return true if the connection should be deleted, false otherwise.
      //
      bool
      release (single_connection*);

    protected:
      details::mutex mutex_;
      single_connection_ptr connection_;
    };

    // Create a new connection every time one is requested.
    //
    class LIBODB_SQLITE_EXPORT new_connection_factory:
      public connection_factory
    {
    public:
      new_connection_factory (): extra_flags_ (0) {}

      virtual connection_ptr
      connect ();

      virtual void
      database (database_type&);

    private:
      new_connection_factory (const new_connection_factory&);
      new_connection_factory& operator= (const new_connection_factory&);

    private:
      int extra_flags_;
    };

    // Pool a number of connections.
    //
    class LIBODB_SQLITE_EXPORT connection_pool_factory:
      public connection_factory
    {
    public:
      // The max_connections argument specifies the maximum number of
      // concurrent connections this pool will maintain. If this value
      // is 0 then the pool will create a new connection every time all
      // of the existing connections are in use.
      //
      // The min_connections argument specifies the minimum number of
      // connections that should be maintained by the pool. If the
      // number of connections maintained by the pool exceeds this
      // number and there are no active waiters for a new connection,
      // then the pool will release the excess connections. If this
      // value is 0 then the pool will maintain all the connections
      // that were ever created.
      //
      // The ping argument specifies whether to ping the connection to
      // make sure it is still alive before returning it to the caller.
      //
      connection_pool_factory (std::size_t max_connections = 0,
                               std::size_t min_connections = 0)
          : max_ (max_connections),
            min_ (min_connections),
            extra_flags_ (0),
            in_use_ (0),
            waiters_ (0),
            cond_ (mutex_)
      {
        // max_connections == 0 means unlimited.
        //
        assert (max_connections == 0 || max_connections >= min_connections);
      }

      virtual connection_ptr
      connect ();

      virtual void
      database (database_type&);

      virtual
      ~connection_pool_factory ();

    private:
      connection_pool_factory (const connection_pool_factory&);
      connection_pool_factory& operator= (const connection_pool_factory&);

    protected:
      class LIBODB_SQLITE_EXPORT pooled_connection: public connection
      {
      public:
        pooled_connection (connection_pool_factory&, int extra_flags = 0);
        pooled_connection (connection_pool_factory&, sqlite3*);

      private:
        static bool
        zero_counter (void*);

      private:
        friend class connection_pool_factory;
        shared_base::refcount_callback cb_;
      };

      friend class pooled_connection;

      typedef details::shared_ptr<pooled_connection> pooled_connection_ptr;
      typedef std::vector<pooled_connection_ptr> connections;

      // This function is called whenever the pool needs to create a new
      // connection.
      //
      virtual pooled_connection_ptr
      create ();

    protected:
      // Return true if the connection should be deleted, false otherwise.
      //
      bool
      release (pooled_connection*);

    protected:
      const std::size_t max_;
      const std::size_t min_;
      int extra_flags_;

      std::size_t in_use_;  // Number of connections currently in use.
      std::size_t waiters_; // Number of threads waiting for a connection.

      connections connections_;

      details::mutex mutex_;
      details::condition cond_;
    };

    class LIBODB_SQLITE_EXPORT default_attached_connection_factory:
      public attached_connection_factory
    {
    public:
      explicit
      default_attached_connection_factory (const connection_ptr& main)
          : attached_connection_factory (main) {}

      using attached_connection_factory::database; // Accessor.

      virtual void
      database (database_type&);

      virtual connection_ptr
      connect ();

      virtual void
      detach ();

      // Active object interface.
      //
      virtual void
      clear ();

      virtual
      ~default_attached_connection_factory ();

    protected:
      static void
      translate_statement (std::string&, const char*, std::size_t, connection&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_CONNECTION_FACTORY_HXX
