// file      : sqlite/transaction/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test esoteric SQLite transaction semantics aspects.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace sqlite = odb::sqlite;
using namespace sqlite;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_specific_database<database> (argc, argv));

    // In SQLite, when a commit fails because of the deferred foreign
    // key constraint violation, the transaction is not automatically
    // rolled back. Make sure we compensate for that.
    //
    try
    {
      object o;
      o.p = odb::lazy_ptr<object1> (*db, 0);

      transaction t (db->begin ());
      db->persist(o);
      t.commit ();
    }
    catch (const odb::exception&)
    {
    }

    // Make sure we can start a new transaction.
    //
    {
      transaction t (db->begin ());
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
