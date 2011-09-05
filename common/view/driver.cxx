// file      : common/view/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test views.
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

    //
    //
    {
      country* ca (new country ("CA", "Canada"));
      country* za (new country ("ZA", "South Africa"));
      country* us (new country ("US", "United States"));
      country* se (new country ("SE", "Sweden"));

      person p1 (1, "John", "Doe", 30, ca);
      person p2 (2, "Jane", "Doe", 29, za);
      person p3 (3, "Joe", "Dirt", 31,  us);
      person p4 (4, "Johansen", "Johansen", 32, se);

      transaction t (db->begin ());
      db->persist (ca);
      db->persist (za);
      db->persist (us);
      db->persist (se);

      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      db->persist (p4);
      t.commit ();
    }

    // view1
    //
    {
      typedef odb::query<view1> query;
      typedef odb::result<view1> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view1> ());
          assert (size (r) == 4);
        }

        {
          result r (db->query<view1> ("ORDER BY age"));
          assert (size (r) == 4);
        }

        {
          result r (db->query<view1> ("age < 31 ORDER BY age"));
          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->first == "Jane" && i->last == "Doe" && i->age == 29);

          assert (++i != r.end ());
          view1 v;
          i.load (v);
          assert (v.first == "John" && v.last == "Doe" && v.age == 30);
        }

        t.commit ();
      }
    }

    // view2
    //
    {
      typedef odb::query<view2> query;
      typedef odb::result<view2> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view2> ());
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->count == 4);
        }

        {
          result r (db->query<view2> ("age < 31"));
          result::iterator i (r.begin ());
          assert (i != r.end ());
          assert (i->count == 2);
        }

        t.commit ();
      }
    }

    // view3
    //
    {
      typedef odb::query<view3> query;
      typedef odb::result<view3> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view3> ());

          size_t count (0);
          for (result::iterator i (r.begin ()); i != r.end (); ++i)
          {
            if (i->last == "Doe")
              assert (i->count == 2);
            else if (i->last == "Dirt" ||
                     i->last == "Johansen")
              assert (i->count == 1);
            else
              assert (false);

            count++;
          }

          assert (count == 3);
        }

        t.commit ();
      }
    }

    // view4
    //
    {
      typedef odb::query<view4> query;
      typedef odb::result<view4> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view4> ("age > 30 ORDER BY age"));

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->first == "Joe" && i->last == "Dirt" &&
                  i->location == "United States");

          assert (++i != r.end ());
          assert (i->first == "Johansen" && i->last == "Johansen" &&
                  i->location == "Sweden");
        }

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
