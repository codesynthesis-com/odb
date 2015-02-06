// file      : odb/oracle/transaction.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_TRANSACTION_HXX
#define ODB_ORACLE_TRANSACTION_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/oracle/version.hxx>
#include <odb/oracle/forward.hxx>
#include <odb/oracle/tracer.hxx>

#include <odb/oracle/details/export.hxx>

namespace odb
{
  namespace oracle
  {
    class transaction_impl;

    class LIBODB_ORACLE_EXPORT transaction: public odb::transaction
    {
    public:
      typedef oracle::database database_type;
      typedef oracle::connection connection_type;

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
      typedef oracle::tracer tracer_type;

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

#include <odb/oracle/transaction.ixx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_TRANSACTION_HXX
