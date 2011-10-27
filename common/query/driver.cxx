// file      : common/query/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query support.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/config.hxx>  // DATABASE_XXX
#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

void
print (result<person>& r)
{
  for (result<person>::iterator i (r.begin ()); i != r.end (); ++i)
  {
    auto_ptr<person> o (i.load ());
    cout << *o << endl;
  }
  cout << endl;
}

const char* names[] = { "John", "Jane", "Joe" };
const char** names_end = names + sizeof (names)/sizeof (names[0]);

const char* key_data[] = { "\x23\x03\x15", "\x13\x13\x54", "\x08\x62\x35" };

int
main (int argc, char* argv[])
{
  vector<char>
    key1 (key_data[0], key_data[0] + 3),
    key2 (key_data[1], key_data[1] + 3),
    key3 (key_data[2], key_data[2] + 3);

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    typedef odb::query<person> query;
    typedef odb::result<person> result;

    //
    //
    {
      person p1 (1, "John", "Doe", 30, true, key1);
      person p2 (2, "Jane", "Doe", 29, true, key2);
      person p3 (3, "Joe", "Dirt", 31, false, key3);
      p3.middle_name_.reset (new string ("Squeaky"));
      person p4 (4, "Johansen", "Johansen", 32, false);
      p4.middle_name_.reset (new string ("J"));

      transaction t (db->begin ());
      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      db->persist (p4);
      t.commit ();
    }

    // Compilation tests.
    //
    if (false)
    {
      string name;
      unsigned short age;

      db->query<person> ("age = " + query::_ref (age));
      db->query<person> ("age = " + query::_val (age));

      query age_q (query::_ref (age) + " = age");
      query name_q ("first = " + query::_val (name));
      query q (age_q + "AND" + name_q);

      db->query<person> (q);
      db->query<person> (age_q + "OR" +
                         name_q + "OR" +
                         "age < " + query::_ref (age));

      query q1 (query::_val (name));
      q1 += " = first";
    }

    // Select-all query.
    //
    cout << "test 001" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> ());

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        person p;
        i.load (p);
        cout << p << endl;
      }

      t.commit ();
    }

    // Select-all query with order by.
    //
    cout << "test 002" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> ("ORDER BY" + query::age));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        person& p (*i);

        cout << p.first_name_;

        if (i->middle_name_.get () != 0)
          cout << ' '  << *i->middle_name_;

        cout << ' ' << i->last_name_ << ' ' << i->age_ <<
          (i->married_ ? " married" : " single") << endl;
      }
      cout << endl;

      t.commit ();
    }

    // String query.
    //
    cout << "test 003" << endl;
    {
      transaction t (db->begin ());
#ifndef DATABASE_ORACLE
      result r (db->query<person> ("age >= 30 AND last = 'Doe'"));
#else
      result r (db->query<person> ("\"age\" >= 30 AND \"last\" = 'Doe'"));
#endif
      print (r);
      t.commit ();
    }

    // Value binding.
    //
    cout << "test 004" << endl;
    {
      transaction t (db->begin ());

      const char* name = "Doe";

#ifndef DATABASE_ORACLE
      result r (
        db->query<person> (
          "age >= " + query::_ref (30) + "AND" +
          "last = " + query::_val (name)));
#else
      result r (
        db->query<person> (
          "\"age\" >= " + query::_ref (30) + "AND" +
          "\"last\" = " + query::_val (name)));
#endif

      print (r);
      t.commit ();
    }

    // Reference binding.
    //
    cout << "test 005" << endl;
    {
      transaction t (db->begin ());

      string name;
      unsigned short age;

#ifndef DATABASE_ORACLE
      query q ("age >= " + query::_ref (age) + "AND" +
               "last = " + query::_ref (name));
#else
      query q ("\"age\" >= " + query::_ref (age) + "AND" +
               "\"last\" = " + query::_ref (name));
#endif

      name = "Doe";
      age = 30;
      result r (db->query<person> (q));
      print (r);

      name = "Dirt";
      age = 31;
      r = db->query<person> (q);
      print (r);

      t.commit ();
    }

    //
    // Language-embedded queries.
    //

    // Compilation tests.
    //
    if (false)
    {
      string name;
      unsigned short age;

      // Column operators.
      //
      query q1 (query::married);
      db->query<person> (query::married);
      db->query<person> (query::married == true);

      //db->query<person> (query::age);

      db->query<person> (query::age == 30);
      db->query<person> (query::age == age);
      db->query<person> (query::age == query::_val (30));
      db->query<person> (query::age == query::_val (age));
      db->query<person> (query::age == query::_ref (age));
      //db->query<person> (query::age == "123");
      //db->query<person> ("123" == query::age);
      //db->query<person> (query::age == query::_val ("123"));
      //db->query<person> (query::age == query::_ref (name));
      db->query<person> (query::last_name == "Doe");
      db->query<person> (query::last_name == name);
      db->query<person> (query::last_name == query::_val ("Doe"));
      db->query<person> (query::last_name == query::_val (name));
      db->query<person> (query::last_name == query::_ref (name));
      //db->query<person> (query::last_name == 30);
      //db->query<person> (query::last_name == query::_val (30));
      //db->query<person> (query::last_name == query::_ref (age));

      db->query<person> (query::last_name.is_null ());
      db->query<person> (query::last_name.is_not_null ());

      db->query<person> (query::first_name == query::last_name);

      db->query<person> (query::first_name.in ("John", "Jane"));
      db->query<person> (query::first_name.in_range (names, names_end));

      // Query operators.
      //
      db->query<person> (query::age == 30 && query::last_name == "Doe");
      db->query<person> (query::age == 30 || query::last_name == "Doe");
      db->query<person> (!(query::age == 30 || query::last_name == "Doe"));
      db->query<person> ((query::last_name == "Doe") + "ORDER BY age");
    }

    // Test is_null/is_not_null.
    //
    cout << "test 006" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::middle_name.is_null ()));
      print (r);
      r = db->query<person> (query::middle_name.is_not_null ());
      print (r);
      t.commit ();
    }

    // Test boolean columns.
    //
    cout << "test 007" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::married));
      print (r);
      r = db->query<person> (!query::married);
      print (r);
      t.commit ();
    }

    // Test implicit by-value, explicit by-value, and by-reference.
    //
    cout << "test 008" << endl;
    {
      string name ("Dirt");

      transaction t (db->begin ());
      result r (db->query<person> (query::last_name == "Doe"));
      print (r);
      r = db->query<person> (query::last_name == query::_val (name));
      print (r);
      query q (query::last_name == query::_ref (name));
      name = "Doe";
      r = db->query<person> (q);
      print (r);
      t.commit ();
    }

    // Test column operators (==, !=, <, >, <=, >=).
    //
    cout << "test 009" << endl;
    {
      transaction t (db->begin ());

      // ==
      //
      result r (db->query<person> (query::last_name == "Doe"));
      print (r);

      // !=
      //
      r = db->query<person> (query::last_name != "Doe");
      print (r);

      // <
      //
      r = db->query<person> (query::age < 31);
      print (r);

      // >
      //
      r = db->query<person> (query::age > 30);
      print (r);

      // <=
      //
      r = db->query<person> (query::age <= 30);
      print (r);

      // >=
      //
      r = db->query<person> (query::age >= 31);
      print (r);

      t.commit ();
    }

    // Test query operators (&&, ||, (), !, +).
    //
    cout << "test 010" << endl;
    {
      transaction t (db->begin ());

      // &&
      //
      result r (db->query<person> (
                  query::last_name == "Doe" && query::age == 29));
      print (r);

      // ||
      //
      r = db->query<person> (query::last_name == "Doe" || query::age == 31);
      print (r);

      // ()
      //
      r = db->query<person> (
        (query::last_name != "Doe" || query::age == 29) && query::married);
      print (r);

      // !=
      //
      r = db->query<person> (!(query::last_name == "Doe"));
      print (r);

      // +
      //
      r = db->query<person> ((query::last_name == "Doe") +
                             "ORDER BY" + query::age);
      print (r);

      t.commit ();
    }

    // Test in/in_range.
    //
    cout << "test 011" << endl;
    {
      transaction t (db->begin ());

      result r (db->query<person> (query::first_name.in ("John", "Jane")));
      print (r);

      r = db->query<person> (query::first_name.in_range (names, names_end));
      print (r);

      t.commit ();
    }

    // Test column-to-column comparison.
    //
    cout << "test 012" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::first_name == query::last_name));
      print (r);
      t.commit ();
    }

    // Test value_traits::type != value_traits::value_type.
    //
    cout << "test 013" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::middle_name == "Squeaky"));
      print (r);
      t.commit ();
    }

    // Test that loading of the same object type during iteration does
    // not invalidate the result.
    //
    cout << "test 014" << endl;
#ifndef DATABASE_ORACLE
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::last_name == "Doe"));

      result::iterator i (r.begin ());
      assert (i != r.end ());
      ++i;
      assert (i != r.end ());
      auto_ptr<person> joe (db->load<person> (3));
      assert (i->last_name_ == "Doe");

      t.commit ();
    }
#endif

    // Test uncached result.
    //
    cout << "test 015" << endl;
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::last_name == "Doe", false));
      print (r);
      t.commit ();
    }

    // Test BLOB column operations.
    //
    cout << "test 016" << endl;
    {
      transaction t (db->begin ());

      result r;
      result::iterator i;

      // Oracle does not support LOB comparisons.
      //
#ifndef DATABASE_ORACLE
      // ==
      //
      r = db->query<person> (query::public_key == key2);

      i = r.begin ();
      assert (i != r.end ());

      assert (i->public_key_ == key2);
      assert (++i == r.end ());
#endif

      // is_null
      //
      r = db->query<person> (query::public_key.is_null ());

      i = r.begin ();
      assert (i != r.end ());

      assert (i->first_name_ == "Johansen" && i->last_name_ == "Johansen");
      assert (++i == r.end ());

      // is_not_null
      //
      r = db->query<person> (query::public_key.is_not_null ());

      i = r.begin ();
      assert (i != r.end ());

      assert (i->first_name_ == "John" && i->last_name_ == "Doe");
      assert (++i != r.end ());

      assert (i->first_name_ == "Jane" && i->last_name_ == "Doe");
      assert (++i != r.end ());

      assert (i->first_name_ == "Joe" && i->last_name_ == "Dirt");
      assert (++i == r.end ());

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
