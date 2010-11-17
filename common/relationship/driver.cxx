// file      : common/relationship/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test object relationships.
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
using namespace odb;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    aggr a ("aggr");
    a.o1 = new obj1 ("o1", "obj1");
    a.o2.reset (new obj2 ("obj2"));
#ifdef HAVE_TR1_MEMORY
    a.o3.reset (new obj3 ("obj3"));

    a.c.num = 123;
    a.c.o3.reset (new obj3 ("c"));

    a.cv.push_back (comp (234, obj3_ptr (new obj3 ("cv 0"))));
    a.cv.push_back (comp (235, obj3_ptr ()));
    a.cv.push_back (comp (236, obj3_ptr (new obj3 ("cv 2"))));
#endif

    a.v1.push_back (new obj1 ("v1 0", "v1 0"));
    a.v1.push_back (0);
    a.v1.push_back (new obj1 ("v1 2", "v1 2"));

    a.s1.insert (new obj1 ("s1 0", "s1 0"));
    a.s1.insert (0);
    a.s1.insert (new obj1 ("s1 2", "s1 2"));

    a.m1[0] = new obj1 ("m1 0", "m1 0");
    a.m1[1] = 0;
    a.m1[2] = new obj1 ("m1 2", "m1 2");

    // persist
    //
    {
      transaction t (db->begin ());
      db->persist (*a.o1);
      db->persist (*a.o2);
#ifdef HAVE_TR1_MEMORY
      db->persist (*a.o3);

      db->persist (*a.c.o3);

      for (comp_vec::iterator i (a.cv.begin ()); i != a.cv.end (); ++i)
        if (i->o3)
          db->persist (*i->o3);
#endif

      for (obj1_vec::iterator i (a.v1.begin ()); i != a.v1.end (); ++i)
        if (*i)
          db->persist (**i);

      for (obj1_set::iterator i (a.s1.begin ()); i != a.s1.end (); ++i)
        if (*i)
          db->persist (**i);

      for (obj1_map::iterator i (a.m1.begin ()); i != a.m1.end (); ++i)
        if (i->second)
          db->persist (*i->second);

      db->persist (a);
      t.commit ();
    }

    // load & compare
    //
    {
      transaction t (db->begin ());
      auto_ptr<aggr> a1 (db->load<aggr> (a.id));
      t.commit ();

      assert (*a1 == a);
    }

    // test NULL pointer
    //
    delete a.o1;
    a.o1 = 0;
    a.o2.reset ();
#ifdef HAVE_TR1_MEMORY
    a.o3.reset ();
#endif

    {
      transaction t (db->begin ());
      db->update (a);
      t.commit ();
    }

    // load & compare
    //
    {
      transaction t (db->begin ());
      auto_ptr<aggr> a1 (db->load<aggr> (a.id));
      t.commit ();

      assert (*a1 == a);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
