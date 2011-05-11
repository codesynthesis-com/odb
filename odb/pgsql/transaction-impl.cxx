// file      : odb/pgsql/transaction-impl.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

#include <libpq-fe.h>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/error.hxx>
#include <odb/pgsql/exceptions.hxx>
#include <odb/pgsql/transaction-impl.hxx>

namespace odb
{
  namespace pgsql
  {
    transaction_impl::
    transaction_impl (database_type& db)
        : odb::transaction_impl (db), connection_ (db.connection ())
    {
      PGresult* r (PQexec (connection_->handle (), "begin"));

      if (!r)
        translate_result_error (*connection_);

      ExecStatusType s (PQresultStatus (r));

      if (PGRES_COMMAND_OK != s)
        translate_result_error (*connection_, r, s);
    }

    transaction_impl::
    ~transaction_impl ()
    {
    }

    void transaction_impl::
    commit ()
    {
      // connection_->clear ();

      PGresult* r (PQexec (connection_->handle (), "commit"));

      if (!r)
        translate_result_error (*connection_);

      ExecStatusType s (PQresultStatus (r));

      if (PGRES_COMMAND_OK != s)
        translate_result_error (*connection_, r, s);

      // Release the connection.
      //
      // connection_.reset ();
    }

    void transaction_impl::
    rollback ()
    {
      // connection_->clear ();

      PGresult* r (PQexec (connection_->handle (), "rollback"));

      if (!r)
        translate_result_error (*connection_);

      ExecStatusType s (PQresultStatus (r));

      if (PGRES_COMMAND_OK != s)
        translate_result_error (*connection_, r, s);

      // Release the connection.
      //
      //connection_.reset ();
    }
  }
}
