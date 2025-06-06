// file      : oracle/custom/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test custom database type mapping in Oracle.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace oracle = odb::oracle;
using namespace oracle;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_specific_database<database> (argc, argv));

    object o (1);
    o.iv.push_back (123);
    o.iv.push_back (234);
    o.iv.push_back (-345);

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      unique_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o == *o1);
    }

    // Update.
    //
    o.iv[0]++;
    o.iv.pop_back ();

    {
      transaction t (db->begin ());
      db->update (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      unique_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o == *o1);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
