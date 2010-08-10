// file      : common/query/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query support.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb;

using odb::shared_ptr;

void
print (result<person>& r)
{
  for (result<person>::iterator i (r.begin ()); i != r.end (); ++i)
  {
    auto_ptr<person> o (*i);
    cout << *o << endl;
  }
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
      person p1 (1, "John", "Doe", 30);
      person p2 (2, "Jane", "Doe", 29);
      person p3 (3, "Joe", "Dirt", 31);

      transaction t (db->begin_transaction ());
      db->persist (p1);
      db->persist (p2);
      db->persist (p3);
      t.commit ();
    }

    // Compilation tests.
    //
    if (false)
    {
      typedef odb::query<person> query;

      string name;
      unsigned short age;

      db->query<person> ("age = " + query::_ref (age));
      db->query<person> ("age = " + query::_val (age));

      query age_q (query::_ref (age) + " = age");
      query name_q ("first_name = " + query::_val (name));
      query q (age_q + "AND" + name_q);

      db->query (q);
      db->query<person> (age_q + "OR" +
                         name_q + "OR" +
                         "age < " + query::_ref (age));
    }

    // Select-all query.
    //
    cout << "test 001" << endl;
    {
      transaction t (db->begin_transaction ());
      result<person> r (db->query<person> ());

      for (result<person>::iterator i (r.begin ()); i != r.end (); ++i)
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
      transaction t (db->begin_transaction ());
      result<person> r (db->query<person> ("ORDER BY age"));
      print (r);
      t.commit ();
    }

    // String query.
    //
    cout << "test 003" << endl;
    {
      transaction t (db->begin_transaction ());
      result<person> r (db->query<person> ("age >= 30 AND last_name = 'Doe'"));
      print (r);
      t.commit ();
    }

    typedef odb::query<person> query;

    // Value binding.
    //
    cout << "test 004" << endl;
    {
      transaction t (db->begin_transaction ());

      const char* name = "Doe";

      result<person> r (
        db->query<person> (
          "age >= " + query::_ref (30) + "AND" +
          "last_name = " + query::_val (name)));

      print (r);
      t.commit ();
    }

    // Reference binding.
    //
    cout << "test 005" << endl;
    {
      transaction t (db->begin_transaction ());

      string name;
      unsigned short age;

      query q ("age >= " + query::_ref (age) + "AND" +
               "last_name = " + query::_ref (name));

      name = "Doe";
      age = 30;
      result<person> r (db->query<person> (q));
      print (r);

      name = "Dirt";
      age = 31;
      r = db->query<person> (q);
      print (r);

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
