// file      : odb/sqlite/connection-factory.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/sqlite/connection-factory.hxx>

#include <utility> // std::move()
#include <cassert>

#include <odb/details/lock.hxx>

#include <odb/sqlite/database.hxx>

#include <odb/sqlite/details/config.hxx> // LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY

using namespace std;

namespace odb
{
  using details::lock;

  namespace sqlite
  {
    //
    // serial_connection_factory
    //

    serial_connection_factory::
    ~serial_connection_factory ()
    {
      // We should hold the last reference to the connection.
      //
      if (connection_ != nullptr)
        assert (connection_.use_count () == 1);
    }

    connection_ptr serial_connection_factory::
    create ()
    {
      return make_shared<connection> (*this);
    }

    connection_ptr serial_connection_factory::
    connect ()
    {
      return connection_;
    }

    void serial_connection_factory::
    database (database_type& db)
    {
      connection_factory::database (db);

      if (!connection_)
        connection_ = create ();
    }

    //
    // single_connection_factory
    //

    single_connection_factory::
    ~single_connection_factory ()
    {
      // If the connection is currently in use, wait for it to return to
      // the factory.
      //
      lock l (mutex_);
    }

    unique_ptr<connection> single_connection_factory::
    create ()
    {
      return unique_ptr<connection> (new connection (*this));
    }

    connection_ptr single_connection_factory::
    connect ()
    {
      mutex_.lock ();

      return shared_ptr<connection> (
        connection_.release (),
        [] (connection* c)
        {
          static_cast<single_connection_factory&> (c->factory_).release (
            unique_ptr<connection> (c));
        });
    }

    void single_connection_factory::
    database (database_type& db)
    {
      connection_factory::database (db);

      if (!connection_)
        connection_ = create ();
    }

    void single_connection_factory::
    release (unique_ptr<connection> c) noexcept
    {
      c->recycle ();
      connection_ = std::move (c);
      mutex_.unlock ();
    }

    //
    // new_connection_factory
    //

    connection_ptr new_connection_factory::
    connect ()
    {
      return make_shared<connection> (*this, extra_flags_);
    }

    void new_connection_factory::
    database (database_type& db)
    {
      bool first (db_ == 0);

      connection_factory::database (db);

      if (!first)
        return;

      // Unless explicitly disabled, enable shared cache.
      //
#if SQLITE_VERSION_NUMBER >= 3006018 && defined(LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY)
      if ((db_->flags () & SQLITE_OPEN_PRIVATECACHE) == 0)
        extra_flags_ |= SQLITE_OPEN_SHAREDCACHE;
#endif
    }

    //
    // connection_pool_factory
    //

    unique_ptr<connection> connection_pool_factory::
    create ()
    {
      return unique_ptr<connection> (new connection (*this, extra_flags_));
    }

    connection_pool_factory::
    ~connection_pool_factory ()
    {
      // Wait for all the connections currently in use to return to the pool.
      //
      lock l (mutex_);
      while (in_use_ != 0)
      {
        waiters_++;
        cond_.wait (l);
        waiters_--;
      }
    }

    connection_ptr connection_pool_factory::
    connect ()
    {
      lock l (mutex_);

      unique_ptr<connection> c;
      while (true)
      {
        // See if we have a spare connection.
        //
        if (connections_.size () != 0)
        {
          c = std::move (connections_.back ());
          connections_.pop_back ();
          break;
        }

        // See if we can create a new one.
        //
        if (max_ == 0 || in_use_ < max_)
        {
          // Make sure we will be able to return it to the pool without
          // allocations (which may throw). See release() for details.
          //
          size_t n (connections_.size () + in_use_ + 1);
          if (min_ == 0 || n <= min_)
            connections_.reserve (n);

          c = create ();
          break;
        }

        // Wait until someone releases a connection.
        //
        waiters_++;
        cond_.wait (l);
        waiters_--;
      }

      in_use_++;

      return shared_ptr<connection> (
        c.release (),
        [] (connection* c)
        {
          static_cast<connection_pool_factory&> (c->factory_).release (
            unique_ptr<connection> (c));
        });
    }

    void connection_pool_factory::
    database (database_type& db)
    {
      bool first (db_ == 0);

      connection_factory::database (db);

      if (!first)
        return;

      // Unless explicitly disabled, enable shared cache.
      //
#if SQLITE_VERSION_NUMBER >= 3006018 && defined(LIBODB_SQLITE_HAVE_UNLOCK_NOTIFY)
      if ((db_->flags () & SQLITE_OPEN_PRIVATECACHE) == 0)
        extra_flags_ |= SQLITE_OPEN_SHAREDCACHE;
#endif

      if (min_ != 0)
      {
        connections_.reserve (min_);

        for(size_t i (0); i < min_; ++i)
          connections_.push_back (create ());
      }
    }

    void connection_pool_factory::
    release (unique_ptr<connection> c) noexcept
    {
      lock l (mutex_);

      // Determine if we need to keep or free this connection.
      //
      bool keep (waiters_ != 0 ||
                 min_ == 0 ||
                 (connections_.size () + in_use_ <= min_));

      in_use_--;

      if (keep)
      {
        c->recycle ();
        connections_.push_back (std::move (c)); // Note: should not allocate.
      }

      if (waiters_ != 0)
        cond_.signal ();
    }

    //
    // default_attached_connection_factory
    //

    void default_attached_connection_factory::
    detach ()
    {
      // Note that this function may be called several times, for example, in
      // case of detach_database() failure.
      //
      if (attached_connection_ != nullptr)
      {
        // We should hold the last reference to the attached connection.
        //
        assert (attached_connection_.use_count () == 1);

        // While it may seem like a good idea to also invalidate query results
        // and reset active statements, if any such result/statement is still
        // alive, then there would be bigger problems since it would have a
        // dangling reference to the connection. In a way, that's the same
        // reason we don't do it in the connection destructor.

        // Remove ourselves from the active object list of the main
        // connection.
        //
        if (next_ != this) // Might have already been done.
          list_remove ();

        const string& s (database ().schema ());

        if (s != "main" && s != "temp")
          main_factory ().detach_database (main_connection_, s);

        // Explicitly free the attached connection so that we don't try to
        // redo this.
        //
        attached_connection_.reset ();
      }
    }

    default_attached_connection_factory::
    ~default_attached_connection_factory ()
    {
      if (attached_connection_ != nullptr)
      {
        // This can throw. Ignoring the failure to detach seems like the most
        // sensible thing to do here.
        //
        try{ detach (); } catch (...) {}
      }
    }

    connection_ptr default_attached_connection_factory::
    connect ()
    {
      return attached_connection_;
    }

    void default_attached_connection_factory::
    database (database_type& db)
    {
      attached_connection_factory::database (db);

      if (!attached_connection_)
      {
        const string& s (db.schema ());

        if (s != "main" && s != "temp")
          main_factory ().attach_database (main_connection_, db.name (), s);

        attached_connection_ = make_shared<connection> (
          *this,
          s != "main" ? &translate_statement : 0);

        // Add ourselves to the active object list of the main connection.
        //
        list_add ();
      }
    }

    void default_attached_connection_factory::
    clear ()
    {
      attached_connection_->clear ();
    }

    void default_attached_connection_factory::
    translate_statement (string& r,
                         const char* text,
                         size_t text_size,
                         connection& conn)
    {
      r.assign (text, text_size);

      // Things will fall apart if any of the statements we translate use
      // "main" as a table alias. So we have this crude check even though it
      // means we cannot use "main" for other aliases (e.g., column).
      //
      assert (r.find ("AS \"main\"") == string::npos);

      const string& s (conn.database ().schema ());
      for (size_t p (0); (p = r.find ("\"main\".", p, 7)) != string::npos; )
      {
        // Verify the preceding character.
        //
        if (p != 0 && r[p - 1] == '.')
        {
          p += 7;
          continue;
        }

        r.replace (p + 1, 4, s);
        p += s.size () + 3;
      }
    }
  }
}
