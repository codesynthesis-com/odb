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
#include <odb/pgsql/result-ptr.hxx>

namespace odb
{
  namespace pgsql
  {
    transaction_impl::
    transaction_impl (database_type& db)
        : odb::transaction_impl (db), connection_ (db.connection ())
    {
      result_ptr r (PQexec (connection_->handle (), "begin"));
      PGresult* h (r.get ());

      if (!h || PGRES_COMMAND_OK != PQresultStatus (h))
        translate_error (*connection_, h);
    }

    transaction_impl::
    ~transaction_impl ()
    {
    }

    void transaction_impl::
    commit ()
    {
      result_ptr r (PQexec (connection_->handle (), "commit"));
      PGresult* h (r.get ());

      if (!h || PGRES_COMMAND_OK != PQresultStatus (h))
        translate_error (*connection_, h);
    }

    void transaction_impl::
    rollback ()
    {
      result_ptr r (PQexec (connection_->handle (), "rollback"));
      PGresult* h (r.get ());

      if (!h || PGRES_COMMAND_OK != PQresultStatus (h))
        translate_error (*connection_, h);
    }
  }
}
