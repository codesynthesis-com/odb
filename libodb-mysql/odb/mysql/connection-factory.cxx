// file      : odb/mysql/connection-factory.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/details/config.hxx>       // ODB_THREADS_*
#include <odb/mysql/details/config.hxx> // LIBODB_MYSQL_THR_KEY_VISIBLE

#if defined(ODB_THREADS_POSIX) && defined(LIBODB_MYSQL_THR_KEY_VISIBLE)
#  include <pthread.h>
#endif

#include <cstdlib> // abort

#include <odb/details/tls.hxx>
#include <odb/details/lock.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/connection-factory.hxx>
#include <odb/mysql/exceptions.hxx>

// This key is in the mysql client library. We use it to resolve the
// following problem: Some pthread implementations zero-out slots that
// don't have destructors during thread termination. As a result, when
// our destructor gets called and we call mysql_thread_end(), the thread-
// specific slot used by MySQL may have been reset to 0 and as a result
// MySQL thinks the data has been freed.
//
// To work around this problem we are going to cache the MySQL's slot
// value and if, during destruction, we see that it is 0, we will restore
// the original value before calling mysql_thread_end(). This will work
// fine for as long as the following conditions are met:
//
// 1. MySQL doesn't use the destructor itself.
// 2. Nobody else tried to call mysql_thread_end() before us.
//
// Note: in 5.7 the key has been made static and is no longer accessible.
//
#if defined(ODB_THREADS_POSIX) && defined(LIBODB_MYSQL_THR_KEY_VISIBLE)
extern pthread_key_t THR_KEY_mysys;
#endif

using namespace std;

namespace odb
{
  using namespace details;

  namespace mysql
  {
    namespace
    {
      static bool main_thread_init_;

      struct mysql_thread_init
      {
#ifndef ODB_THREADS_NONE
        mysql_thread_init ()
            : init_ (false)
        {
          if (!main_thread_init_)
          {
            if (::mysql_thread_init ())
            {
              throw database_exception (
                CR_UNKNOWN_ERROR, "?????", "thread initialization failed");
            }

            init_ = true;

#if defined(ODB_THREADS_POSIX) && defined(LIBODB_MYSQL_THR_KEY_VISIBLE)
            value_ = pthread_getspecific (THR_KEY_mysys);
#endif
          }
        }

        ~mysql_thread_init ()
        {
          if (init_)
          {
#if defined(ODB_THREADS_POSIX) && defined(LIBODB_MYSQL_THR_KEY_VISIBLE)
            if (pthread_getspecific (THR_KEY_mysys) == 0)
              pthread_setspecific (THR_KEY_mysys, value_);
#endif
            mysql_thread_end ();
          }
        }

      private:
        bool init_;
#if defined(ODB_THREADS_POSIX) && defined(LIBODB_MYSQL_THR_KEY_VISIBLE)
        void* value_;
#endif
#endif // ODB_THREADS_NONE
      };

      static ODB_TLS_OBJECT (mysql_thread_init) mysql_thread_init_;

      struct mysql_process_init
      {
        mysql_process_init ()
        {
          // Force allocation of our thread-specific key before THR_KEY_mysys
          // in MySQL. This will (hopefully) get us the desired order of TLS
          // destructor calls (i.e., our destructor before zeroing-out the
          // THR_KEY_mysys value). This is pretty much the only way (except
          // maybe guessing the THR_KEY_mysys value) to get clean thread
          // termination if THR_KEY_mysys symbol is hidden, as is the case
          // in the Fedora build of libmysqlclient. See also the comment
          // at the beginning of this file.
          //
          main_thread_init_ = true;
          tls_get (mysql_thread_init_);
          main_thread_init_ = false;

          if (mysql_library_init (0 ,0, 0))
            abort ();
        }

        ~mysql_process_init ()
        {
          mysql_library_end ();

          // Finalize the main thread now in case TLS destruction
          // doesn't happen for the main thread.
          //
          tls_free (mysql_thread_init_);
        }
      };

      static mysql_process_init mysql_process_init_;
    }

    // new_connection_factory
    //
    connection_ptr new_connection_factory::
    connect ()
    {
      tls_get (mysql_thread_init_);

      return connection_ptr (new (shared) connection (*this));
    }

    // connection_pool_factory
    //
    connection_pool_factory::pooled_connection_ptr connection_pool_factory::
    create ()
    {
      return pooled_connection_ptr (new (shared) pooled_connection (*this));
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
      tls_get (mysql_thread_init_);

      // The outer loop checks whether the connection we were
      // given is still valid.
      //
      while (true)
      {
        pooled_connection_ptr c;

        lock l (mutex_);

        // The inner loop tries to find a free connection.
        //
        while (true)
        {
          // See if we have a spare connection.
          //
          if (connections_.size () != 0)
          {
            c = connections_.back ();
            connections_.pop_back ();

            c->callback_ = &c->cb_;
            in_use_++;
            break;
          }

          // See if we can create a new one.
          //
          if(max_ == 0 || in_use_ < max_)
          {
            // For new connections we don't need to ping so we
            // can return immediately.
            //
            c = create ();
            c->callback_ = &c->cb_;
            in_use_++;
            return c;
          }

          // Wait until someone releases a connection.
          //
          waiters_++;
          cond_.wait (l);
          waiters_--;
        }

        l.unlock ();

        if (!ping_ || c->ping ())
          return c;
      }

      return pooled_connection_ptr (); // Never reached.
    }

    void connection_pool_factory::
    database (database_type& db)
    {
      tls_get (mysql_thread_init_);

      bool first (db_ == 0);

      connection_factory::database (db);

      if (!first)
        return;

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
      c->clear ();
      c->callback_ = 0;

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
    pooled_connection (connection_pool_factory& f)
        : connection (f)
    {
      cb_.arg = this;
      cb_.zero_counter = &zero_counter;
    }

    connection_pool_factory::pooled_connection::
    pooled_connection (connection_pool_factory& f, MYSQL* handle)
        : connection (f, handle)
    {
      cb_.arg = this;
      cb_.zero_counter = &zero_counter;
    }

    bool connection_pool_factory::pooled_connection::
    zero_counter (void* arg)
    {
      pooled_connection* c (static_cast<pooled_connection*> (arg));
      return static_cast<connection_pool_factory&> (c->factory_).release (c);
    }
  }
}
