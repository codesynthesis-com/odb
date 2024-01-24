// file      : odb/sqlite/connection.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <new>    // std::bad_alloc
#include <string>
#include <cassert>

#include <odb/details/lock.hxx>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/transaction.hxx>
#include <odb/sqlite/statement.hxx>
#include <odb/sqlite/statement-cache.hxx>
#include <odb/sqlite/prepared-query.hxx>
#include <odb/sqlite/error.hxx>
#include <odb/sqlite/exceptions.hxx> // deadlock

#include <odb/sqlite/details/config.hxx> // LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY

using namespace std;

extern "C" void
odb_sqlite_connection_unlock_callback (void**, int);

namespace odb
{
  using namespace details;

  namespace sqlite
  {
    connection::
    connection (connection_factory& cf,
                int extra_flags,
                statement_translator* st)
        : odb::connection (cf),
          statement_translator_ (st),
          unlock_cond_ (unlock_mutex_),
          active_objects_ (0)
    {
      database_type& db (database ());

      int f (db.flags () | extra_flags);
      const string& n (db.name ());

      // If we are opening a temporary database, then add the create flag.
      //
      if (n.empty () || n == ":memory:")
        f |= SQLITE_OPEN_CREATE;

      // A connection can only be used by a single thread at a time. So
      // disable locking in SQLite unless explicitly requested.
      //
#if defined(SQLITE_OPEN_NOMUTEX)
      if ((f & SQLITE_OPEN_FULLMUTEX) == 0)
        f |= SQLITE_OPEN_NOMUTEX;
#endif

      sqlite3* h (0);

      // sqlite3_open_v2() was only addedin SQLite 3.5.0.
      //
#if SQLITE_VERSION_NUMBER >= 3005000
      const string& vfs (db.vfs ());
      int e (
        sqlite3_open_v2 (
          n.c_str (), &h, f, (vfs.empty () ? 0 : vfs.c_str ())));
#else
      // Readonly opening not supported in SQLite earlier than 3.5.0.
      //
      assert ((f & SQLITE_OPEN_READONLY) == 0);
      int e (sqlite3_open (n.c_str (), &h));
#endif

      handle_.reset (h);

      if (e != SQLITE_OK)
      {
        if (handle_ == 0)
          throw bad_alloc ();

        translate_error (e, *this);
      }

      init ();
    }

    connection::
    connection (connection_factory& cf,
                sqlite3* handle,
                statement_translator* st)
        : odb::connection (cf),
          handle_ (handle),
          statement_translator_ (st),
          unlock_cond_ (unlock_mutex_),
          active_objects_ (0)
    {
      init ();
    }

    void connection::
    init ()
    {
      database_type& db (database ());

      // Enable/disable foreign key constraints.
      //
      generic_statement st (
        *this,
        db.foreign_keys ()
        ? "PRAGMA foreign_keys=ON"
        : "PRAGMA foreign_keys=OFF",
        db.foreign_keys () ? 22 : 23);
      st.execute ();

      // String lengths include '\0', as per the SQLite manual suggestion.
      //
      begin_.reset (new (shared) generic_statement (*this, "BEGIN", 6));
      commit_.reset (new (shared) generic_statement (*this, "COMMIT", 7));
      rollback_.reset (new (shared) generic_statement (*this, "ROLLBACK", 9));

      // Create statement cache.
      //
      statement_cache_.reset (new statement_cache_type (*this));
    }

    connection::
    connection (attached_connection_factory& cf, statement_translator* st)
        : odb::connection (cf),
          handle_ (0),
          statement_translator_ (st),
          unlock_cond_ (unlock_mutex_),
          active_objects_ (0)
    {
      // Copy some things over from the main connection.
      //
      connection& main (*cf.main_connection_);

      tracer_ = main.tracer_;

      // Create statement cache.
      //
      statement_cache_.reset (new statement_cache_type (*this));
    }

    connection::
    ~connection ()
    {
      // Destroy prepared query statements before freeing the connections.
      //
      recycle ();
      clear_prepared_map ();
    }

    generic_statement& connection::
    begin_statement ()
    {
      return static_cast<generic_statement&> (*begin_);
    }

    generic_statement& connection::
    begin_immediate_statement ()
    {
      if (!begin_immediate_)
        begin_immediate_.reset (
          new (shared) generic_statement (*this, "BEGIN IMMEDIATE", 16));

      return static_cast<generic_statement&> (*begin_immediate_);
    }

    generic_statement& connection::
    begin_exclusive_statement ()
    {
      if (!begin_exclusive_)
        begin_exclusive_.reset (
          new (shared) generic_statement (*this, "BEGIN EXCLUSIVE", 16));

      return static_cast<generic_statement&> (*begin_exclusive_);
    }

    generic_statement& connection::
    commit_statement ()
    {
      return static_cast<generic_statement&> (*commit_);
    }

    generic_statement& connection::
    rollback_statement ()
    {
      return static_cast<generic_statement&> (*rollback_);
    }

    transaction_impl* connection::
    begin ()
    {
      return new transaction_impl (
        connection_ptr (inc_ref (this)), transaction_impl::deferred);
    }

    transaction_impl* connection::
    begin_immediate ()
    {
      return new transaction_impl (
        connection_ptr (inc_ref (this)), transaction_impl::immediate);
    }

    transaction_impl* connection::
    begin_exclusive ()
    {
      return new transaction_impl (
        connection_ptr (inc_ref (this)), transaction_impl::exclusive);
    }

    unsigned long long connection::
    execute (const char* s, std::size_t n)
    {
      generic_statement st (*this, s, n);
      return st.execute ();
    }

    inline void
    connection_unlock_callback (void** args, int n)
    {
      for (int i (0); i < n; ++i)
      {
        connection* c (static_cast<connection*> (args[i]));
        details::lock l (c->unlock_mutex_);
        c->unlocked_ = true;
        c->unlock_cond_.signal ();
      }
    }

    void connection::
    wait ()
    {
#ifdef LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY
      unlocked_ = false;

      // unlock_notify() returns SQLITE_OK or SQLITE_LOCKED (deadlock).
      //
      int e (sqlite3_unlock_notify (handle (),
                                    &odb_sqlite_connection_unlock_callback,
                                    this));
      if (e == SQLITE_LOCKED)
        throw deadlock ();

      details::lock l (unlock_mutex_);

      while (!unlocked_)
        unlock_cond_.wait (l);
#else
      translate_error (SQLITE_LOCKED, *this);
#endif
    }

    void connection::
    clear ()
    {
      invalidate_results ();

      // The current first active_object may remove itself from the list and
      // make the second object (if any) the new first.
      //
      for (active_object** pp (&active_objects_); *pp != 0; )
      {
        active_object* p (*pp);
        p->clear ();

        // Move to the next object if this one decided to stay on the list.
        //
        if (*pp == p)
          pp = &p->next_;
      }
    }

    // connection_factory
    //
    connection_factory::
    ~connection_factory ()
    {
    }

    void connection_factory::
    database (database_type& db)
    {
      odb::connection_factory::db_ = &db;
      db_ = &db;
    }

    void connection_factory::
    attach_database (const connection_ptr& conn,
                     const std::string& name,
                     const std::string& schema)
    {
      conn->execute ("ATTACH DATABASE '" + name + "' AS \"" + schema + '"');
    }

    void connection_factory::
    detach_database (const connection_ptr& conn, const std::string& schema)
    {
      conn->execute ("DETACH DATABASE \"" + schema + '"');
    }
  }
}

extern "C" void
odb_sqlite_connection_unlock_callback (void** args, int n)
{
  odb::sqlite::connection_unlock_callback (args, n);
}
