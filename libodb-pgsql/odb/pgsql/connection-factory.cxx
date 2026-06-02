// file      : odb/pgsql/connection-factory.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/pgsql/connection-factory.hxx>

#include <utility> // std::move()

#include <odb/pgsql/exceptions.hxx>

#include <odb/details/lock.hxx>

using namespace std;

namespace odb
{
  using details::lock;

  namespace pgsql
  {
    // new_connection_factory
    //
    connection_ptr new_connection_factory::
    connect ()
    {
      return make_shared<connection> (*this);
    }

    // connection_pool_factory
    //
    unique_ptr<connection> connection_pool_factory::
    create ()
    {
      return unique_ptr<connection> (new connection (*this));
    }

    connection_pool_factory::
    ~connection_pool_factory ()
    {
      // Wait for all the connections currently in use to return to
      // the pool.
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
          // This is tricky: since we may not be actually destroying the
          // connection, its enable_shared_from_this::weak_this member may
          // continue holding a reference to the control block, preventing it
          // form being freed.
          //
          // A problem in both libstdc++ (as of GCC 16, bug #125565) and
          // libc++ (as of Clang 23).
          //
#if 0
//#ifndef NDEBUG
          weak_ptr<odb::connection> w (c->weak_from_this ()), e;
          assert (!w.owner_before (e) && !e.owner_before (w));
#endif
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

      if (min_ != 0)
      {
        connections_.reserve (min_);

        for (size_t i (0); i < min_; ++i)
          connections_.push_back (create ());
      }
    }

    void connection_pool_factory::
    release (unique_ptr<connection> c) noexcept
    {
      lock l (mutex_);

      // Determine if we need to keep or free this connection.
      //
      bool keep (!c->failed () &&
                 (waiters_ != 0 ||
                  min_ == 0 ||
                  (connections_.size () + in_use_ <= min_)));

      in_use_--;

      if (keep)
      {
        c->recycle ();
        connections_.push_back (std::move (c)); // Note: should not allocate.
      }

      if (waiters_ != 0)
        cond_.signal ();
    }
  }
}
