// file      : odb/mssql/transaction-impl.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/tracer.hxx>

#include <odb/mssql/mssql.hxx>
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

      // In ODBC a transaction is started automatically before the first
      // statement is executed.
      //
    }

    void transaction_impl::
    commit ()
    {
      // Invalidate query results.
      //
      connection_->invalidate_results ();

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "COMMIT");
      }

      SQLRETURN r (
        SQLEndTran (SQL_HANDLE_DBC, connection_->handle (), SQL_COMMIT));

      if (!SQL_SUCCEEDED (r))
        translate_error (r, *connection_, true);

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

      {
        odb::tracer* t;
        if ((t = connection_->tracer ()) || (t = database_.tracer ()))
          t->execute (*connection_, "ROLLBACK");
      }

      SQLRETURN r (
        SQLEndTran (SQL_HANDLE_DBC, connection_->handle (), SQL_ROLLBACK));

      if (!SQL_SUCCEEDED (r))
        translate_error (r, *connection_, true);

      // Release the connection.
      //
      connection_.reset ();
    }
  }
}
