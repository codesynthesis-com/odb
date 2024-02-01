// file      : odb/mssql/transaction.ixx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction-impl.hxx>

namespace odb
{
  namespace mssql
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
      // mssql::transaction.
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
      return static_cast<connection_type&> (odb::transaction::connection ());
    }

    inline transaction::connection_type& transaction::
    connection (odb::database& db)
    {
      return static_cast<connection_type&> (odb::transaction::connection (db));
    }

    inline void transaction::
    current (transaction& t)
    {
      odb::transaction::current (t);
    }
  }
}
