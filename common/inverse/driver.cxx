// file      : common/inverse/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test bidirectional relationships with inverse sides.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/session.hxx>

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

    // Naked pointer version.
    //
    {
      obj1_ptr o1 (new obj1);
      obj2_ptr o2 (new obj2);
      obj3_ptr o3_1 (new obj3);
      obj3_ptr o3_2 (new obj3);

      o1->id = "obj1";
      o1->o2 = o2;
      o1->o3.insert (o3_1);
      o1->o3.insert (o3_2);

      o2->str = "obj2";
      o2->o1 = o1;

      o3_1->str = "obj3 1";
      o3_1->o1 = o1;

      o3_2->str = "obj3 3";
      o3_2->o1 = o1;

      // persist
      //
      {
        transaction t (db->begin ());

        // obj2 and obj3 come first to get object id assigned.
        //
        db->persist (o3_1);
        db->persist (o3_2);
        db->persist (o2);
        db->persist (o1);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        obj2_ptr x2 (db->load<obj2> (o2->id));
        obj3_ptr x3_1 (db->load<obj3> (o3_1->id));
        obj3_ptr x3_2 (db->load<obj3> (o3_2->id));
        t.commit ();

        assert (x2->str == o2->str);
        assert (x2->o1->id == o1->id);
        assert (x2->o1->o2 == x2);

        assert (x3_1->str == o3_1->str);
        assert (x3_2->str == o3_2->str);
        assert (x3_1->o1 == x3_2->o1);
        assert (x3_1->o1->id == o1->id);
        assert (x3_1->o1->o3.find (x3_1) != x3_1->o1->o3.end ());
        assert (x3_1->o1->o3.find (x3_2) != x3_1->o1->o3.end ());

        delete x2->o1;
      }

      delete o1;
    }

    // TR1 pointer version.
    //
#ifdef HAVE_TR1_MEMORY
    {
      tr1_obj1_ptr o1 (new tr1_obj1);
      tr1_obj2_ptr o2 (new tr1_obj2);
      tr1_obj3_ptr o3_1 (new tr1_obj3);
      tr1_obj3_ptr o3_2 (new tr1_obj3);

      o1->id = "obj1";
      o1->o2 = o2;
      o1->o3.push_back (o3_1);
      o1->o3.push_back (o3_2);

      o2->str = "obj2";
      o2->o1 = o1;

      o3_1->str = "obj3 1";
      o3_1->o1 = o1;

      o3_2->str = "obj3 3";
      o3_2->o1 = o1;

      // persist
      //
      {
        transaction t (db->begin ());

        // obj2 and obj3 come first to get object id assigned.
        //
        db->persist (o3_1);
        db->persist (o3_2);
        db->persist (o2);
        db->persist (o1);

        t.commit ();
      }

      // load
      //
      {
        session s;
        transaction t (db->begin ());
        tr1_obj2_ptr x2 (db->load<tr1_obj2> (o2->id));
        tr1_obj3_ptr x3_1 (db->load<tr1_obj3> (o3_1->id));
        tr1_obj3_ptr x3_2 (db->load<tr1_obj3> (o3_2->id));
        t.commit ();

        assert (x2->str == o2->str);
        assert (x2->o1.lock ()->id == o1->id);
        assert (x2->o1.lock ()->o2 == x2);

        assert (x3_1->str == o3_1->str);
        assert (x3_2->str == o3_2->str);
        assert (x3_1->o1.lock () == x3_2->o1.lock ());
        assert (x3_1->o1.lock ()->id == o1->id);
        assert (x3_1->o1.lock ()->o3[0] == x3_1);
        assert (x3_1->o1.lock ()->o3[1] == x3_2);
      }
    }
#endif
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
