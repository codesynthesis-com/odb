// file      : common/object/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test persistent classes.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    // Test persistent class template instantiation.
    //
    {
      using namespace test1;

      pair_object po;
      po.second = "abc";

      derived d;
      d.x = "abc";
      d.n = 123;

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist<pair_object> (po);
        db->persist (d);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        unique_ptr<pair_object> po1 (db->load<pair_object> (po.first));
        unique_ptr<derived> d1 (db->load<derived> (d.id));
        t.commit ();

        assert (po == *po1);

        assert (d.x == d1->x);
        assert (d.n == d1->n);
      }

      // Test the API confusion.
      //
      {
        transaction t (db->begin ());
        db->update<pair_object> (po);
        db->reload<pair_object> (po);
        db->erase<pair_object> (po);

        db->query<pair_object> ();
        t.commit ();
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
