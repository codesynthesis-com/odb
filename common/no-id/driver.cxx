// file      : common/no-id/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test persistent classes without id.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    object o1 (1, "aaa");
    object o2 (2, "bbb");
    object o3 (3, "ccc");

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (o1);
      db->persist (o2);
      db->persist (o2); // Ok, since there is no id.
      db->persist (o3);
      t.commit ();
    }

    // Compile errors.
    //
    {
      //db->load<object> (1);
      //db->find<object> (1);
      //db->update (o1);
      //db->erase<object> (1);
    }

    typedef odb::query<object> query;
    typedef odb::result<object> result;

    // Query.
    //
    {
      transaction t (db->begin ());

      {
        result r (db->query<object> ());
        assert (size (r) == 4);
      }

      {
        result r (db->query<object> (query::str == "aaa"));
        result::iterator i (r.begin ());
        assert (i != r.end ());
        assert (i->num == 1 && i->str == "aaa");
        object o;
        i.load (o);
        //i.id (); // Compile-time error.
        assert (o.num == 1 && o.str == "aaa");
        assert (++i == r.end ());
      }

      {
        result r (db->query<object> (query::num < 3));
        assert (size (r) == 3);
      }

      t.commit ();
    }

    // Erase (query).
    //
    {
      transaction t (db->begin ());
      assert (db->erase_query<object> (query::num == 2) == 2);
      assert (db->erase_query<object> () == 2);
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
