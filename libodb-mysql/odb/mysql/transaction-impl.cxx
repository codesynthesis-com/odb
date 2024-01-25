// file      : odb/mysql/transaction-impl.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/tracer.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/database.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/error.hxx>
#include <odb/mysql/transaction-impl.hxx>

namespace odb
{
  namespace mysql
  {
    transaction_impl::
    transaction_impl (database_type& db)
        : odb::transaction_impl (db)
    {
    }

    transaction_impl::
    transaction_impl (connection_ptr c)
        : odb::transaction_impl (c->database (), *c), connection_ (c)
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

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "BEGIN");
      }

      if (mysql_real_query (connection_->handle (), "begin", 5) != 0)
        translate_error (*connection_);
    }

    void transaction_impl::
    commit ()
    {
      // Invalidate query results.
      //
      connection_->invalidate_results ();

      // Cancel and clear the active statement if any. This normally
      // should happen automatically, however, if an exception is
      // thrown, this may not be the case.
      //
      connection_->clear ();

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "COMMIT");
      }

      if (mysql_real_query (connection_->handle (), "commit", 6) != 0)
        translate_error (*connection_);

      // Release the connection.
      //
      connection_.reset ();
    }

    void transaction_impl::
    rollback ()
    {
      // Invalidate query results.
      //
      connection_->invalidate_results ();

      // Cancel and clear the active statement if any. This normally
      // should happen automatically, however, if an exception is
      // thrown, this may not be the case.
      //
      connection_->clear ();

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "ROLLBACK");
      }

      if (mysql_real_query (connection_->handle (), "rollback", 8) != 0)
        translate_error (*connection_);

      // Release the connection.
      //
      connection_.reset ();
    }
  }
}
