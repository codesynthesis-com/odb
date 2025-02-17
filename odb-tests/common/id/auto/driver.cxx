// file      : common/id/auto/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test automatic id assignment.
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

    // object
    //
    {
      unsigned long id1, id2, id3;
      {
        object o1 ("one");
        object o2 ("two");
        object o3 ("three");

        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();

        id1 = o1.id_;
        id2 = o2.id_;
        id3 = o3.id_;

        assert (id1 != id2);
        assert (id1 != id3);
        assert (id2 != id3);
      }

      {
        transaction t (db->begin ());
        unique_ptr<object> o1 (db->load<object> (id1));
        unique_ptr<object> o2 (db->load<object> (id2));
        unique_ptr<object> o3 (db->load<object> (id3));
        t.commit ();

        assert (o1->id_ == id1 && o1->str_ == "one");
        assert (o2->id_ == id2 && o2->str_ == "two");
        assert (o3->id_ == id3 && o3->str_ == "three");
      }
    }

    // auto_only
    //
    {
      unsigned short id;
      {
        auto_only o;

        transaction t (db->begin ());
        db->persist (o);
        t.commit ();

        id = o.id_;
      }

      {
        transaction t (db->begin ());
        unique_ptr<auto_only> o (db->load<auto_only> (id));
        t.commit ();

        assert (o->id_ == id);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
