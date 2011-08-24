// file      : odb/pgsql/transaction.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_TRANSACTION_HXX
#define ODB_PGSQL_TRANSACTION_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class transaction_impl;

    class LIBODB_PGSQL_EXPORT transaction: public odb::transaction
    {
    public:
      typedef pgsql::database database_type;
      typedef pgsql::connection connection_type;

      explicit
      transaction (transaction_impl*);

      // Return the database this transaction is on.
      //
      database_type&
      database ();

      // Return the underlying database connection for this transaction.
      //
      connection_type&
      connection ();

      // Return current transaction or throw if there is no transaction
      // in effect.
      //
      static transaction&
      current ();

      // Set the current thread's transaction.
      //
      static void
      current (transaction&);

    public:
      transaction_impl&
      implementation ();
    };
  }
}

#include <odb/pgsql/transaction.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_TRANSACTION_HXX
