// file      : odb/oracle/transaction.ixx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction-impl.hxx>

namespace odb
{
  namespace oracle
  {
    inline transaction::
    transaction (transaction_impl* impl, bool make_current)
        : odb::transaction (impl, make_current)
    {
    }

    inline transaction::
    transaction ()
        : odb::transaction ()
    {
    }

    inline transaction_impl& transaction::
    implementation ()
    {
      // We can use static_cast here since we have an instance of
      // oracle::transaction.
      //
      return static_cast<transaction_impl&> (
        odb::transaction::implementation ());
    }

    inline transaction::database_type& transaction::
    database ()
    {
      return static_cast<database_type&> (odb::transaction::database ());
    }

    inline transaction::connection_type& transaction::
    connection ()
    {
      return implementation ().connection ();
    }

    inline void transaction::
    current (transaction& t)
    {
      odb::transaction::current (t);
    }
  }
}
