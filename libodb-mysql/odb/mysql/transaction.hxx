// file      : odb/mysql/transaction.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_TRANSACTION_HXX
#define ODB_MYSQL_TRANSACTION_HXX

#include <odb/pre.hxx>

#include <odb/transaction.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/tracer.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class transaction_impl;

    class LIBODB_MYSQL_EXPORT transaction: public odb::transaction
    {
    public:
      typedef mysql::database database_type;
      typedef mysql::connection connection_type;

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
      typedef mysql::tracer tracer_type;

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

#include <odb/mysql/transaction.ixx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_TRANSACTION_HXX
