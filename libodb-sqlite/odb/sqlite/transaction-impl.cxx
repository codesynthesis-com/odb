// file      : odb/sqlite/transaction-impl.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <sqlite3.h>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/statement.hxx>
#include <odb/sqlite/transaction-impl.hxx>

namespace odb
{
  namespace sqlite
  {
    transaction_impl::
    transaction_impl (database_type& db, lock l)
        : odb::transaction_impl (db), lock_ (l)
    {
    }

    transaction_impl::
    transaction_impl (connection_ptr c, lock l)
        : odb::transaction_impl (c->database (), *c),
          connection_ (c),
          lock_ (l)
    {
    }

    transaction_impl::
    ~transaction_impl ()
    {
    }

    void transaction_impl::
    start ()
    {
      // Grab a connection if we don't already have one.
      //
      if (connection_ == 0)
      {
        connection_ = static_cast<database_type&> (database_).connection ();
        odb::transaction_impl::connection_ = connection_.get ();
      }

      connection_type& mc (connection_->main_connection ());

      switch (lock_)
      {
      case deferred:
        {
          mc.begin_statement ().execute ();
          break;
        }
      case immediate:
        {
          mc.begin_immediate_statement ().execute ();
          break;
        }
      case exclusive:
        {
          mc.begin_exclusive_statement ().execute ();
          break;
        }
      }
    }

    // In SQLite, when a commit fails (e.g., because of the deferred
    // foreign key constraint violation), the transaction may not
    // be automatically rolled back. So we have to do it ourselves.
    //
    struct commit_guard
    {
      commit_guard (connection& c): c_ (&c) {}
      void release () {c_ = 0;}

      ~commit_guard ()
      {
        if (c_ != 0 && sqlite3_get_autocommit (c_->handle ()) == 0)
        {
          // This is happening while another exception is active.
          //
          try
          {
            c_->rollback_statement ().execute ();
          }
          catch (...) {}
        }
      }

    private:
      connection* c_;
    };

    void transaction_impl::
    commit ()
    {
      connection_type& mc (connection_->main_connection ());

      // Invalidate query results and reset active statements.
      //
      // Active statements will prevent COMMIT from completing (write
      // statements) or releasing the locks (read statements). Normally, a
      // statement is automatically reset on completion, however, if an
      // exception is thrown, that may not happen.
      //
      // Note: must be done via the main connection.
      //
      mc.clear ();

      {
        commit_guard cg (mc);
        mc.commit_statement ().execute ();
        cg.release ();
      }

      // Release the connection.
      //
      connection_.reset ();
    }

    void transaction_impl::
    rollback ()
    {
      connection_type& mc (connection_->main_connection ());

      // Invalidate query results and reset active statements (the same
      // reasoning as in commit()).
      //
      // Note: must be done via the main connection.
      //
      mc.clear ();

      mc.rollback_statement ().execute ();

      // Release the connection.
      //
      connection_.reset ();
    }

    odb::connection& transaction_impl::
    connection (odb::database* pdb)
    {
      if (pdb == 0)
        return *connection_;

      // Pick the corresponding connection for main/attached database.
      //
      database_type& db (static_cast<database_type&> (*pdb));

      assert (&db.main_database () ==
              &static_cast<database_type&> (database_).main_database ());

      return db.schema ().empty ()
        ? connection_->main_connection ()
        : *static_cast<attached_connection_factory&> (*db.factory_).attached_connection_;
    }

    // Store transaction tracer in the main connection.
    //
    void transaction_impl::
    tracer (odb::tracer* t)
    {
      connection_->main_connection ().transaction_tracer_ = t;
    }

    odb::tracer* transaction_impl::
    tracer () const
    {
      return connection_->main_connection ().transaction_tracer_;
    }
  }
}
