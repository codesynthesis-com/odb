// file      : odb/mssql/transaction.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_TRANSACTION_HXX
#define ODB_MSSQL_TRANSACTION_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/mssql/version.hxx>
#include <odb/mssql/forward.hxx>
#include <odb/mssql/tracer.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class transaction_impl;

    class LIBODB_MSSQL_EXPORT transaction: public odb::transaction
    {
    public:
      typedef mssql::database database_type;
      typedef mssql::connection connection_type;

      explicit
      transaction (transaction_impl*, bool make_current = true);

      transaction ();

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

      // SQL statement tracing.
      //
    public:
      typedef mssql::tracer tracer_type;

      void
      tracer (tracer_type& t)
      {
        odb::transaction::tracer (t);
      }

      void
      tracer (tracer_type* t)
      {
        odb::transaction::tracer (t);
      }

      using odb::transaction::tracer;

    public:
      transaction_impl&
      implementation ();
    };
  }
}

#include <odb/mssql/transaction.ixx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_TRANSACTION_HXX
