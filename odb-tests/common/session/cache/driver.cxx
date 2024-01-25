// file      : common/session/cache/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test session object cache.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
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

    // Test the session_required exception.
    //
    {
      using namespace test1;

      shared_ptr<obj1> o1a (new obj1 (1));
      shared_ptr<obj1> o1b (new obj1 (2));
      shared_ptr<obj2> o2 (new obj2 (1));

      o1a->o2 = o2;
      o1b->o2 = o2;

      o2->o1.push_back (o1a);
      o2->o1.push_back (o1b);

      {
        transaction t (db->begin ());
        db->persist (o1a);
        db->persist (o1b);
        db->persist (o2);
        t.commit ();
      }

      {
        transaction t (db->begin ());

        try
        {
          shared_ptr<obj1> o1 (db->load<obj1> (1));
          assert (false);
        }
        catch (const session_required&)
        {
        }

        t.commit ();
      }

      {
        session s;
        transaction t (db->begin ());
        shared_ptr<obj1> o1 (db->load<obj1> (1));
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
