// file      : odb/mssql/transaction-impl.ixx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace mssql
  {
    inline transaction_impl::connection_type& transaction_impl::
    connection ()
    {
      return *connection_;
    }
  }
}
