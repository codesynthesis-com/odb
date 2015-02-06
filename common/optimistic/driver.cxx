// file      : common/optimistic/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test optimistic concurrency support.
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

unsigned long
version (const auto_ptr<database>& db, unsigned long id)
{
  typedef odb::query<object_version> query;
  typedef odb::result<object_version> result;

  result r (db->query<object_version> (query::id == id));
  return r.empty () ? 0 : r.begin ()->ver;
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    object o (1);
    o.num = 123;
    o.str = "abc";

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    // Verify initial version in the instance and database.
    //
    assert (o.ver == 1);
    {
      transaction t (db->begin ());
      assert (version (db, 1) == 1);
      t.commit ();
    }

    object c (o);
    o.num++;
    o.str += 'd';

    {
      transaction t (db->begin ());
      db->update (o);
      t.commit ();
    }

    // Verify updated version in the instance and database.
    //
    assert (o.ver == 2);
    {
      transaction t (db->begin ());
      assert (version (db, 1) == 2);
      t.commit ();
    }

    // Verify the data has been updated.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      t.commit ();

      assert (o1->ver == 2 && o1->num == 124 && o1->str == "abcd");
    }

    // Try to update using outdated object.
    //
    c.num--;
    c.str += 'z';

    {
      transaction t (db->begin ());

      try
      {
        db->update (c);
        assert (false);
      }
      catch (const object_changed&) {}

      // Verify the data hasn't changed.
      //
      auto_ptr<object> o1 (db->load<object> (1));
      assert (o1->ver == 2 && o1->num == 124 && o1->str == "abcd");

      // Reload the object.
      //
      db->reload (c);
      assert (c.ver == 2 && c.num == 124);

      // Check that we don't reload an object that is up-to-date.
      //
      c.num--;
      db->reload (c);
      assert (c.ver == 2 && c.num == 123);

      t.commit ();
    }

    // Try to delete using an outdated object.
    //
    {
      transaction t (db->begin ());

      try
      {
        db->update (o);
        db->erase (c);
        assert (false);
      }
      catch (const object_changed&) {}

      t.commit ();
    }

    // Try to delete using an up-to-date object.
    //
    {
      transaction t (db->begin ());
      db->erase (o);
      t.commit ();
    }

    // Try to update deleted object.
    //
    {
      transaction t (db->begin ());

      try
      {
        db->update (o);
        assert (false);
      }
      catch (const object_not_persistent&)
      {
        assert (false);
      }
      catch (const object_changed&) {}

      t.commit ();
    }

    // Optimistic delete of objects with container requires
    // extra logic. Test it here.
    //
    {
      container o ("abc");
      o.nums.push_back (1);
      o.nums.push_back (2);
      o.nums.push_back (3);

      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      container c (o);
      o.nums.pop_back ();

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      // Try to delete using an outdated object.
      //
      {
        transaction t (db->begin ());

        try
        {
          db->erase (c);
          assert (false);
        }
        catch (const object_changed&) {}

        // Verify the container data hasn't changed.
        //
        auto_ptr<container> o1 (db->load<container> ("abc"));
        assert (o1->nums.size () == 2 && o1->nums[0] == 1 && o1->nums[1] == 2);

        t.commit ();
      }

      // Try to delete using an up-to-date object.
      //
      {
        transaction t (db->begin ());
        db->erase (o);
        t.commit ();
      }
    }

    // Test optimistic class inheritance. This is a shortened version
    // of the object test.
    //
    {
      derived o;
      o.num = 123;
      o.str = "abc";

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      derived c (o);
      o.num++;
      o.str += 'd';

      {
        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      // Try to update using outdated object.
      //
      c.num--;
      c.str += 'z';

      {
        transaction t (db->begin ());

        try
        {
          db->update (c);
          assert (false);
        }
        catch (const object_changed&) {}

        // Reload the object.
        //
        db->reload (c);
        assert (c.ver == 2 && c.num == 124);

        t.commit ();
      }

      // Try to delete using an outdated object.
      //
      {
        transaction t (db->begin ());

        try
        {
          db->update (o);
          db->erase (c);
          assert (false);
        }
        catch (const object_changed&) {}

        t.commit ();
      }

      // Try to delete using an up-to-date object.
      //
      {
        transaction t (db->begin ());
        db->erase (o);
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
