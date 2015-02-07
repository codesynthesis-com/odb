// file      : common/view/basics/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test view basics.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>
#include <common/config.hxx> // DATABASE_XXX

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
    result r;
    if (db->id () != odb::id_oracle)
      r = db->query<V> ("age < 31");
    else
      r = db->query<V> ("\"age\" < 31");

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

  {
    auto_ptr<V> v (db->query_one<V> ());
    assert (v->count == 4);
  }

  {
    auto_ptr<V> v;
    if (db->id () != odb::id_oracle)
      v.reset (db->query_one<V> ("age < 31"));
    else
      v.reset (db->query_one<V> ("\"age\" < 31"));
    assert (v->count == 2);
  }

  {
    auto_ptr<V> v (db->query_one<V> (query::age < 31));
    assert (v->count == 2);
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
    result r;
    if (db->id () != odb::id_oracle)
      r = db->query<V> ((query::person::age > 30) + "ORDER BY age");
    else
      r = db->query<V> ((query::person::age > 30) + "ORDER BY \"age\"");

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
                (query::person::age > 30) +
                "ORDER BY " + query::person::age));

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
      typedef odb::result<view1> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view1> ());
          assert (size (r) == 4);
        }

        {
          result r;
          if (db->id () != odb::id_oracle)
            r = db->query<view1> ("ORDER BY age");
          else
            r = db->query<view1> ("ORDER BY \"age\"");

          assert (size (r) == 4);
        }

        {
          result r;
          if (db->id () != odb::id_oracle)
            r = db->query<view1> ("age < 31 ORDER BY age");
          else
            r = db->query<view1> ("\"age\" < 31 ORDER BY \"age\"");

          view1_check (r);
        }

        t.commit ();
      }
    }

    // view1a
    //
    {
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
      typedef odb::result<view1b> result;

      {
        transaction t (db->begin ());

        result r (db->query<view1b> ());
        view1_check (r);

        t.commit ();
      }

      // No native parameter support in dynamic multi-database mode.
      //
#ifndef DATABASE_COMMON
      {
        typedef odb::query<view1b> query;

        transaction t (db->begin ());

#ifndef DATABASE_ORACLE
        result r (db->query<view1b> ("first = " + query::_val ("Jane")));
#else
        result r (db->query<view1b> ("\"first\" = " + query::_val ("Jane")));
#endif

        result::iterator i (r.begin ());

        assert (i != r.end ());
        assert (i->first == "Jane" && i->last == "Doe");
        assert (++i == r.end ());

        t.commit ();
      }
#endif
    }

    // view1c
    //
    {
      typedef odb::result<view1c> result;

      {
        transaction t (db->begin ());

        result r;
        if (db->id () != odb::id_oracle)
          r = db->query<view1c> ("SELECT first, last, age "
                                 "FROM t_view_b_person "
                                 "WHERE age < 31 ORDER BY age");
        else
          r = db->query<view1c> ("SELECT \"first\", \"last\", \"age\" "
                                 "FROM \"t_view_b_person\" "
                                 "WHERE \"age\" < 31 ORDER BY \"age\"");
        view1_check (r);

        t.commit ();
      }
    }

    // view1d
    //
    {
      typedef odb::result<view1d> result;

      {
        transaction t (db->begin ());

        {
          result r;
          if (db->id () != odb::id_oracle)
            r = db->query<view1d> ("age < 31 ORDER BY age");
          else
            r = db->query<view1d> ("\"age\" < 31 ORDER BY \"age\"");

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

    // No native parameter support in dynamic multi-database mode.
    //
#ifndef DATABASE_COMMON
    view6_test<view6c> (
#ifndef DATABASE_ORACLE
      db, "e.name = " + odb::query<view6c>::_val ("Simple Tech, Inc"));
#else
      db, "\"e\".\"name\" = " + odb::query<view6c>::_val ("Simple Tech, Inc"));
#endif
#endif

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

    // view13
    //
    {
      typedef odb::query<view13> query;
      typedef odb::result<view13> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<view13> (
                      (query::person::age < 32) +
                      "ORDER BY" + query::employer::name));

          assert (size (r) == 2);
        }

        t.commit ();
      }
    }

    // view14
    //
    {
      transaction t (db->begin ());
      assert (size (db->query<view14> ()) == 2);
      t.commit ();
    }

    // Test join types.
    //
    {
      using namespace test2;

      {
        obj1 o11 (1, 1);
        obj1 o12 (2, 2);

        obj2 o21 (1, 1);
        obj2 o22 (2, 1);
        obj2 o23 (3, 3);

        transaction t (db->begin ());
        db->persist (o11);
        db->persist (o12);
        db->persist (o21);
        db->persist (o22);
        db->persist (o23);
        t.commit ();
      }

      {
        typedef odb::query<vleft> query;
        typedef odb::result<vleft> result;

        transaction t (db->begin ());
        result r (db->query<vleft> (
                    "ORDER BY" + query::o1::id1 + "," + query::o2::id2));
        result::iterator i (r.begin ());
        assert (  i != r.end () && i->id1 == 1 && *i->id2 == 1);
        assert (++i != r.end () && i->id1 == 1 && *i->id2 == 2);
        assert (++i != r.end () && i->id1 == 2 && i->id2.null ());
        assert (++i == r.end ());
        t.commit ();
      }

#if !defined(DATABASE_SQLITE) && !defined(DATABASE_COMMON)
      {
        typedef odb::query<vright> query;
        typedef odb::result<vright> result;

        transaction t (db->begin ());
        result r (db->query<vright> (
                    "ORDER BY" + query::o1::id1 + "," + query::o2::id2));
        result::iterator i (r.begin ());
        assert (  i != r.end () && i->id1 == 1 && *i->id2 == 1);
        assert (++i != r.end () && i->id1 == 1 && *i->id2 == 2);
        assert (++i != r.end () && i->id1 == 2 && i->id2.null ());
        assert (++i == r.end ());
        t.commit ();
      }
#endif

#if !defined(DATABASE_MYSQL) &&  \
    !defined(DATABASE_SQLITE) && \
    !defined(DATABASE_COMMON)
      {
        typedef odb::query<vfull> query;
        typedef odb::result<vfull> result;

        transaction t (db->begin ());
        result r (db->query<vfull> (
                    "ORDER BY" + query::o1::id1 + "," + query::o2::id2));
        result::iterator i (r.begin ());

        // SQL Server orders NULL values first. Got to be different.
        //
#ifdef DATABASE_MSSQL
        assert (  i != r.end () && i->id1.null () && *i->id2 == 3);
        assert (++i != r.end () && *i->id1 == 1 && *i->id2 == 1);
        assert (++i != r.end () && *i->id1 == 1 && *i->id2 == 2);
        assert (++i != r.end () && *i->id1 == 2 && i->id2.null ());
#else
        assert (  i != r.end () && *i->id1 == 1 && *i->id2 == 1);
        assert (++i != r.end () && *i->id1 == 1 && *i->id2 == 2);
        assert (++i != r.end () && *i->id1 == 2 && i->id2.null ());
        assert (++i != r.end () && i->id1.null () && *i->id2 == 3);
#endif
        assert (++i == r.end ());
        t.commit ();
      }
#endif

      {
        typedef odb::query<vinner> query;
        typedef odb::result<vinner> result;

        transaction t (db->begin ());
        result r (db->query<vinner> (
                    "ORDER BY" + query::o1::id1 + "," + query::o2::id2));
        result::iterator i (r.begin ());
        assert (  i != r.end () && i->id1 == 1 && i->id2 == 1);
        assert (++i != r.end () && i->id1 == 1 && i->id2 == 2);
        assert (++i == r.end ());
        t.commit ();
      }

      {
        typedef odb::query<vcross> query;
        typedef odb::result<vcross> result;

        transaction t (db->begin ());
        result r (db->query<vcross> (
                    "ORDER BY" + query::o1::id1 + "," + query::o2::id2));
        result::iterator i (r.begin ());
        assert (  i != r.end () && i->id1 == 1 && i->id2 == 1);
        assert (++i != r.end () && i->id1 == 1 && i->id2 == 2);
        assert (++i != r.end () && i->id1 == 1 && i->id2 == 3);
        assert (++i != r.end () && i->id1 == 2 && i->id2 == 1);
        assert (++i != r.end () && i->id1 == 2 && i->id2 == 2);
        assert (++i != r.end () && i->id1 == 2 && i->id2 == 3);
        assert (++i == r.end ());
        t.commit ();
      }

      // Inner JOIN via relationship/container.
      //
      {
        obj3 o31 (1, 1);
        obj3 o32 (2, 2);

        obj4 o41 (1, 1);
        obj4 o42 (2, 2);
        o42.o3.push_back (&o32);

        transaction t (db->begin ());
        db->persist (o31);
        db->persist (o32);
        db->persist (o41);
        db->persist (o42);
        t.commit ();
      }

      {
        typedef odb::result<vrel> result;

        transaction t (db->begin ());
        result r (db->query<vrel> ());
        result::iterator i (r.begin ());
        assert (  i != r.end () && i->id4 == 2);
        assert (++i == r.end ());
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
