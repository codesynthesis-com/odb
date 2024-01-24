// file      : tests/basics/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Basic test to make sure the library is usable. Functionality testing
// is done in the odb-tests package.

#include <cassert>
#include <sstream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/exceptions.hxx>
#include <odb/sqlite/transaction.hxx>

using namespace odb::sqlite;

int
main ()
{
  {
    std::ostringstream os;
    database::print_usage (os);
    assert (!os.str ().empty ());
  }

  database db (":memory:");

  {
    transaction t (db.begin ());
    db.execute ("CREATE TABLE test (id INTEGER PRIMARY KEY, str TEXT)");
    t.commit ();
  }

  {
    transaction t (db.begin ());
    db.execute ("INSERT INTO test VALUES (123, 'abc')");
    t.commit ();
  }

  try
  {
    transaction t (db.begin ());
    db.execute ("INSERT INTO test VALUES (123, 'ABC')");
    assert (false);
  }
  catch (const database_exception&) {}

  {
    transaction t (db.begin ());
    db.execute ("DROP TABLE test");
    t.commit ();
  }
}
