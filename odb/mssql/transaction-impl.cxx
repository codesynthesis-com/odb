// file      : odb/mssql/transaction-impl.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/tracer.hxx>

#include <odb/mssql/database.hxx>
#include <odb/mssql/connection.hxx>
#include <odb/mssql/transaction-impl.hxx>
#include <odb/mssql/error.hxx>

namespace odb
{
  namespace mssql
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
      /*
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

      if (mssql_real_query (connection_->handle (), "begin", 5) != 0)
        translate_error (*connection_);
      */
    }

    void transaction_impl::
    commit ()
    {
      /*
      connection_->clear ();

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "COMMIT");
      }

      if (mssql_real_query (connection_->handle (), "commit", 6) != 0)
        translate_error (*connection_);

      // Release the connection.
      //
      //connection_.reset ();
      */
    }

    void transaction_impl::
    rollback ()
    {
      /*
      connection_->clear ();

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "ROLLBACK");
      }

      if (mssql_real_query (connection_->handle (), "rollback", 8) != 0)
        translate_error (*connection_);

      // Release the connection.
      //
      //connection_.reset ();
      */
    }
  }
}
