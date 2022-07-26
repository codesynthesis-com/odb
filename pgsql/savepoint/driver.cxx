// file      : pgsql/savepoint/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test transaction savepoints.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    {
      object o1 (1);
      object o2 (2);

      transaction t (db->begin ());
      db->persist (o1);

      /*
      try
      {
        db->persist (o1);
        assert (false);
      }
      catch (const odb::object_already_persistent&)
      {
      }
      */

      //db->persist (o2);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
