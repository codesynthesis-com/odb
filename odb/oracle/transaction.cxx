// file      : odb/oracle/transaction.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cassert>

#include <odb/oracle/transaction.hxx>

namespace odb
{
  namespace oracle
  {
    transaction& transaction::
    current ()
    {
      // While the impl type can be of the concrete type, the transaction
      // object can be created as either odb:: or odb::oracle:: type. To
      // work around that we are going to hard-cast one to the other
      // relying on the fact that they have the same representation and
      // no virtual functions. The former is checked in the tests.
      //
      odb::transaction& b (odb::transaction::current ());
      assert (dynamic_cast<transaction_impl*> (&b.implementation ()) != 0);
      return reinterpret_cast<transaction&> (b);
    }
  }
}
