// file      : odb/oracle/transaction-impl.ixx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    inline transaction_impl::connection_type& transaction_impl::
    connection ()
    {
      return *connection_;
    }
  }
}
