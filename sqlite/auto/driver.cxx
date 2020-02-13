// file      : sqlite/auto/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test manual/automatic id assignment.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/sqlite/database.hxx>
#include <odb/sqlite/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace sqlite = odb::sqlite;
using namespace sqlite;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    // object
    //
    {
      unsigned long id1, id2, id3;
      {
        object o1 (1, "one");
        object o2 ("two");
        object o3 (3, "three");

        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();

        id1 = *o1.id_;
        id2 = *o2.id_;
        id3 = *o3.id_;

        assert (id1 == 1);
        assert (id3 == 3);

        assert (id2 != id1);
        assert (id2 != id3);
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> o1 (db->load<object> (id1));
        auto_ptr<object> o2 (db->load<object> (id2));
        auto_ptr<object> o3 (db->load<object> (id3));
        t.commit ();

        assert (*o1->id_ == id1 && o1->str_ == "one");
        assert (*o2->id_ == id2 && o2->str_ == "two");
        assert (*o3->id_ == id3 && o3->str_ == "three");
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
