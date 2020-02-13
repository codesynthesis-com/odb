// file      : tests/basics/driver.cxx
// license   : ODB NCUEL; see accompanying LICENSE file

// Basic test to make sure the library is usable. Functionality testing
// is done in the odb-tests package.

#include <cassert>
#include <sstream>

#include <odb/oracle/database.hxx>
#include <odb/oracle/exceptions.hxx>
#include <odb/oracle/transaction.hxx>

using namespace odb::oracle;

int
main ()
{
  {
    std::ostringstream os;
    database::print_usage (os);
    assert (!os.str ().empty ());
  }

  // We can't really do much here since that would require a database. We can
  // create a fake database object as long as we don't expect to get a valid
  // connection.
  //
  database db ("john", "secret", "dummy whammy");

  try
  {
    transaction t (db.begin ());
    assert (false);
  }
  catch (const database_exception&) {}
}
