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

template <typename V>
void
view1_check (odb::result<V>& r)
{
  typedef odb::result<V> result;

  typename result::iterator i (r.begin ());

  assert (i != r.end ());
  assert (i->first == "Jane" && i->last == "Doe" && i->age == 29);

  assert (++i != r.end ());
  V v;
  i.load (v);
  assert (v.first == "John" && v.last == "Doe" && v.age == 30);

  assert (++i == r.end ());
}

template <typename V>
void
view2_test (const auto_ptr<database>& db)
{
  typedef odb::query<V> query;
  typedef odb::result<V> result;
  typedef typename result::iterator iterator;

  transaction t (db->begin ());

  {
    result r (db->query<V> ());
    iterator i (r.begin ());
    assert (i != r.end ());
    assert (i->count == 4);
  }

  {
    result r (db->query<V> ("age < 31"));
    iterator i (r.begin ());
    assert (i != r.end ());
    assert (i->count == 2);
  }

  {
    result r (db->query<V> (query::age < 31));
    iterator i (r.begin ());
    assert (i != r.end ());
    assert (i->count == 2);
  }

  t.commit ();
}

template <typename V>
void
view4_test (const auto_ptr<database>& db)
{
  typedef odb::query<V> query;
  typedef odb::result<V> result;
  typedef typename result::iterator iterator;

  transaction t (db->begin ());

  {
    result r (db->query<V> ("age > 30 ORDER BY age"));

    iterator i (r.begin ());

    assert (i != r.end ());
    assert (i->first_name == "Joe" && i->last_name == "Dirt" &&
            i->name == "United States");

    assert (++i != r.end ());
    assert (i->first_name == "Johan" && i->last_name == "Johansen" &&
            i->name == "Sweden");

    assert (++i == r.end ());
  }

  {
    result r (db->query<V> (
                (query::person::age > 30) + "ORDER BY age"));

    iterator i (r.begin ());

    assert (i != r.end ());
    assert (i->first_name == "Joe" && i->last_name == "Dirt" &&
            i->name == "United States");

    assert (++i != r.end ());
    assert (i->first_name == "Johan" && i->last_name == "Johansen" &&
            i->name == "Sweden");

    assert (++i == r.end ());
  }

  {
    result r (db->query<V> (query::residence::code == "US"));

    iterator i (r.begin ());

    assert (i != r.end ());
    assert (i->first_name == "Joe" && i->last_name == "Dirt" &&
            i->name == "United States");

    assert (++i == r.end ());
  }

  t.commit ();
}

template <typename V>
void
view6_test (const auto_ptr<database>& db, const odb::query<V>& q)
{
  typedef odb::query<V> query;
  typedef odb::result<V> result;
  typedef typename result::iterator iterator;

  transaction t (db->begin ());

  {
    result r (db->query<V> (q));

    iterator i (r.begin ());

    assert (i != r.end ());
    assert (i->first_name == "John" && i->last_name == "Doe" &&
            i->employer == "Simple Tech, Inc");

    assert (++i != r.end ());
    assert (i->first_name == "Joe" && i->last_name == "Dirt" &&
            i->employer == "Simple Tech, Inc");

    assert (++i == r.end ());
  }

  t.commit ();
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    //
    //
    {
      country ca ("CA", "Canada");
      country za ("ZA", "South Africa");
      country us ("US", "United States");
      country se ("SE", "Sweden");

      person p1 (1, "John", "Doe", 30, male, measures (60, 160), &ca, &ca);
      person p2 (2, "Jane", "Doe", 29, female, measures (70, 170), &za, &us);
      person p3 (3, "Joe", "Dirt", 31, male, measures (80, 180), &us, &za);
      person p4 (4, "Johan", "Johansen", 32, male, measures (90, 190), &se,
                 &se);

      p2.husband = &p1;

      employer st ("Simple Tech, Inc");
      employer ct ("Complex Tech, Inc");

      p2.previous_employer = st.name;
      p3.previous_employer = ct.name;

      st.employees.push_back (&p1);
      st.employees.push_back (&p3);
      st.head_count = 2;

      ct.employees.push_back (&p2);
      ct.employees.push_back (&p4);
      ct.head_count = 2;

      transaction t (db->begin ());
      db->persist (ca);
      db->persist (za);
      db->persist (us);
      db->persist (se);

      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      db->persist (p4);

      db->persist (st);
      db->persist (ct);
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
          view1_check (r);
        }

        t.commit ();
      }
    }

    // view1a
    //
    {
      typedef odb::query<view1a> query;
      typedef odb::result<view1a> result;

      {
        transaction t (db->begin ());

        result r (db->query<view1a> ());
        view1_check (r);

        t.commit ();
      }
    }

    // view1b
    //
    {
      typedef odb::query<view1b> query;
      typedef odb::result<view1b> result;

      {
        {
          transaction t (db->begin ());

          result r (db->query<view1b> ());
          view1_check (r);

          t.commit ();
        }

        {
          transaction t (db->begin ());

          result r (db->query<view1b> ("first = " + query::_val ("Jane")));
          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->first == "Jane" && i->last == "Doe");
          assert (++i == r.end ());

          t.commit ();
        }
      }
    }

    // view1c
    //
    {
      typedef odb::query<view1c> query;
      typedef odb::result<view1c> result;

      {
        transaction t (db->begin ());

        result r (
          db->query<view1c> (
            "SELECT first, last, age "
            "FROM t_view_person "
            "WHERE age < 31 ORDER BY age"));

        view1_check (r);

        t.commit ();
      }
    }

    // view1d
    //
    {
      typedef odb::query<view1d> query;
      typedef odb::result<view1d> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view1d> ("age < 31 ORDER BY age"));
          view1_check (r);
        }

        t.commit ();
      }
    }

    // view2
    //
    view2_test<view2> (db);
    view2_test<view2a> (db);
    view2_test<view2b> (db);
    view2_test<view2c> (db);

    // view3
    //
    {
      typedef odb::query<view3> query;
      typedef odb::result<const view3> result; // Test const result.

      {
        transaction t (db->begin ());

        {
          result r (db->query<view3> ());

          size_t count (0);
          for (result::iterator i (r.begin ()); i != r.end (); ++i)
          {
            if (i->last_name == "Doe")
              assert (i->count == 2);
            else if (i->last_name == "Dirt" ||
                     i->last_name == "Johansen")
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

    // view3a
    //
    {
      typedef odb::query<view3a> query;
      typedef odb::result<view3a> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view3a> (query::last_name == "Doe"));
          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->last_name == "Doe" && i->count == 2);
          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view4
    //
    view4_test<view4> (db);
    view4_test<view4a> (db);

    // view5
    //
    {
      typedef odb::query<view5> query;
      typedef odb::result<view5> result;

      {
        transaction t (db->begin ());

        {
          result r (
            db->query<view5> (
              query::residence::name == query::nationality::name));

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->first_name == "John" && i->last_name == "Doe" &&
                  i->rname == "Canada" && i->rname == "Canada");

          assert (++i != r.end ());
          assert (i->first_name == "Johan" && i->last_name == "Johansen" &&
                  i->rname == "Sweden" && i->rname == "Sweden");

          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view6
    //
    view6_test<view6> (
      db, odb::query<view6>::employer::name == "Simple Tech, Inc");

    view6_test<view6a> (
      db, odb::query<view6a>::employer::name == "Simple Tech, Inc");

    view6_test<view6b> (
      db, odb::query<view6b>::employer::name == "Simple Tech, Inc");

    view6_test<view6c> (
      db, "e.name = " + odb::query<view6c>::_val ("Simple Tech, Inc"));

    // view7
    //
    {
      typedef odb::query<view7> query;
      typedef odb::result<view7> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view7> (query::person::last_name == "Doe"));

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->first_name == "Jane" && i->last_name == "Doe" &&
                  !i->head_count.null () && *i->head_count == 2);

          assert (++i != r.end ());
          assert (i->first_name == "John" && i->last_name == "Doe" &&
                  i->head_count.null ());

          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view8
    //
    {
      typedef odb::query<view8> query;
      typedef odb::result<view8> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view8> ());

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->wife_name == "Jane" && i->husb_name == "John");
          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view9
    //
    {
      typedef odb::query<view9> query;
      typedef odb::result<view9> result;

      {
        transaction t (db->begin ());

        {
          // Test case-insensitive clause prefix detection.
          //
          result r (db->query<view9> ("where" + (query::gender == female)));

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->first_name == "Jane" && i->last_name == "Doe" &&
                  i->gender == female);
          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view10
    //
    {
      typedef odb::query<view10> query;
      typedef odb::result<view10> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view10> (
                      query::measures.weight > 60 &&
                      query::measures.hight < 190));

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->last_name == "Doe" &&
                  i->measures.weight == 70 && i->measures.hight == 170);

          assert (++i != r.end ());
          assert (i->last_name == "Dirt" &&
                  i->measures.weight == 80 && i->measures.hight == 180);

          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view11
    //
    {
      typedef odb::query<view11> query;
      typedef odb::result<view11> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view11> ());

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->last_name == "Doe" && i->hight == 170);

          assert (++i != r.end ());
          assert (i->last_name == "Dirt" && i->hight == 180);

          assert (++i == r.end ());
        }

        t.commit ();
      }
    }

    // view12
    //
    {
      typedef odb::query<view12> query;
      typedef odb::result<view12> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view12> (query::last_name == "Dirt"));

          result::iterator i (r.begin ());

          assert (i != r.end ());
          assert (i->residence == "US");
          assert (++i == r.end ());
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
