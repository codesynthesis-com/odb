// file      : odb/pgsql/transaction.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_TRANSACTION_HXX
#define ODB_PGSQL_TRANSACTION_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx>
#include <odb/pgsql/tracer.hxx>

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

      connection_type&
      connection (odb::database&);

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
      typedef pgsql::tracer tracer_type;

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

#include <odb/pgsql/transaction.ixx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_TRANSACTION_HXX
