// file      : odb/mssql/transaction.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <odb/mssql/transaction.hxx>

namespace odb
{
  namespace mssql
  {
    transaction& transaction::
    current ()
    {
      // While the impl type can be of the concrete type, the transaction
      // object can be created as either odb:: or odb::mssql:: type. To
      // work around that we are going to hard-cast one to the other
      // relying on the fact that they have the same representation and
      // no virtual functions. The former is checked in the tests.
      //
      odb::transaction& b (odb::transaction::current ());
      dynamic_cast<transaction_impl&> (b.implementation ());
      return reinterpret_cast<transaction&> (b);
    }
  }
}
