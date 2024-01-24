// file      : tests/basics/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Basic test to make sure the library is usable. Functionality testing
// is done in the odb-tests package.

#include <cassert>

#include <odb/exceptions.hxx>
#include <odb/transaction.hxx>

using namespace odb;

int
main ()
{
  // Transaction.
  //
  {
    assert (!transaction::has_current ());

    try
    {
      transaction::current ();
      assert(false);
    }
    catch (const not_in_transaction&) {}
  }
}
