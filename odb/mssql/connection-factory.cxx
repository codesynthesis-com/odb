// file      : odb/mssql/connection-factory.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/connection-factory.hxx>
#include <odb/mssql/exceptions.hxx>

#include <odb/details/lock.hxx>

using namespace std;

namespace odb
{
  using namespace details;

  namespace mssql
  {
    //
    // connection_factory
    //

    connection_factory::
    ~connection_factory ()
    {
    }

    //
    // new_connection_factory
    //

    connection_ptr new_connection_factory::
    connect ()
    {
      return connection_ptr (new (shared) connection (*db_));
    }

    void new_connection_factory::
    database (database_type& db)
    {
      db_ = &db;
    }

    //
    // connection_pool_factory
    //

    connection_pool_factory::pooled_connection_ptr connection_pool_factory::
    create ()
    {
      return pooled_connection_ptr (new (shared) pooled_connection (*db_));
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
        cond_.wait ();
        waiters_--;
      }
    }

    connection_ptr connection_pool_factory::
    connect ()
    {
      lock l (mutex_);

      while (true)
      {
        // See if we have a spare connection.
        //
        if (connections_.size () != 0)
        {
          shared_ptr<pooled_connection> c (connections_.back ());
          connections_.pop_back ();

          c->pool_ = this;
          in_use_++;
          return c;
        }

        // See if we can create a new one.
        //
        if (max_ == 0 || in_use_ < max_)
        {
          shared_ptr<pooled_connection> c (create ());
          c->pool_ = this;
          in_use_++;
          return c;
        }

        // Wait until someone releases a connection.
        //
        waiters_++;
        cond_.wait ();
        waiters_--;
      }
    }

    void connection_pool_factory::
    database (database_type& db)
    {
      db_ = &db;

      if (min_ > 0)
      {
        connections_.reserve (min_);

        for(size_t i (0); i < min_; ++i)
          connections_.push_back (create ());
      }
    }

    bool connection_pool_factory::
    release (pooled_connection* c)
    {
      c->pool_ = 0;

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
        connections_.push_back (pooled_connection_ptr (inc_ref (c)));
        connections_.back ()->recycle ();
      }

      if (waiters_ != 0)
        cond_.signal ();

      return !keep;
    }

    //
    // connection_pool_factory::pooled_connection
    //

    connection_pool_factory::pooled_connection::
    pooled_connection (database_type& db)
        : connection (db), pool_ (0)
    {
      callback_.arg = this;
      callback_.zero_counter = &zero_counter;
      shared_base::callback_ = &callback_;
    }

    connection_pool_factory::pooled_connection::
    pooled_connection (database_type& db, SQLHDBC handle)
        : connection (db, handle), pool_ (0)
    {
      callback_.arg = this;
      callback_.zero_counter = &zero_counter;
      shared_base::callback_ = &callback_;
    }

    bool connection_pool_factory::pooled_connection::
    zero_counter (void* arg)
    {
      pooled_connection* c (static_cast<pooled_connection*> (arg));
      return c->pool_ ? c->pool_->release (c) : true;
    }
  }
}
