// file      : odb/mssql/transaction-impl.ixx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

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
