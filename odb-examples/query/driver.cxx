// file      : query/driver.cxx
// copyright : not copyrighted - public domain

#include <vector>
#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include "database.hxx" // create_database

#include "person.hxx"
#include "person-odb.hxx"

using namespace std;

using odb::database;
using odb::transaction;

typedef odb::query<person> query;
typedef odb::result<person> result;

static void
print (person& p)
{
  cout << p.first () << " ";

  if (!p.middle ().null ())
    cout << p.middle ().get () << " ";

  cout << p.last () << " " << p.age () << endl;
}

static void
print (result& r)
{
  for (result::iterator i (r.begin ()); i != r.end (); ++i)
    print (*i);

  cout << endl;
}

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    {
      typedef vector<person> people;

      people p;

      p.push_back (person ("John", "Doe", 21));
      p.push_back (person ("John", "Smith", 22));
      p.push_back (person ("Jack", "Johnson", 31));
      p.push_back (person ("John", "JJ", "Jackson", 32));
      p.push_back (person ("Jane", "JD", "Doe", 23));
      p.push_back (person ("Jane", "JS", "Smith", 24));

      transaction t (db->begin ());

      for (people::iterator i (p.begin ()); i != p.end (); ++i)
        db->persist (*i);

      t.commit ();
    }

    // A simple query and result handling.
    //
    {
      transaction t (db->begin ());

      result r (db->query<person> (query::age < 30));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        cout << i->first () << " " << i->last () << " " << i->age () << endl;
      }

      // Alternatively we can get a dynamically-allocated object.
      //
      /*
      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        unique_ptr<person> p (i.load ());
        cout << p->first () << " " << p->last () << " " << p->age () << endl;
      }
      */

      // Or we can load the state into an existing object.
      //
      /*
      person p ("", "", 0);
      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        i.load (p);
        cout << p.first () << " " << p.last () << " " << p.age () << endl;
      }
      */

      cout << endl;

      t.commit ();
    }

    // Use query_one() as a shortcut when there's no more than one element
    // in the result.
    //
    {
      transaction t (db->begin ());

      unique_ptr<person> p (db->query_one<person> (query::age == 21));

      if (p.get () != 0)
      {
        print (*p);
        cout << endl;
      }

      // Or we can load the state into an existing object.
      //
      /*
      person p ("", "", 0);

      if (db->query_one<person> (query::age == 21, p))
      {
        print (p);
        cout << endl;
      }
      */

      t.commit ();
    }

    // Query that shows how to combine expressions with &&, ||, and !
    // as well as use paranthesis to control evaluation order.
    //
    {
      transaction t (db->begin ());

      result r (
        db->query<person> (
          (query::first == "John" || query::first == "Jane") &&
          !(query::last != "Doe" || query::age > 30)));

      print (r);
      t.commit ();
    }

    // Query that shows how to use by-reference parameter binding.
    //
    {
      transaction t (db->begin ());

      unsigned short lower (0), upper (0);

      query q (query::age >= query::_ref (lower) &&
               query::age < query::_ref (upper));

      for (unsigned short band (0); band < 10; ++band)
      {
        lower = band * 10;
        upper = lower + 10;

        result r (db->query<person> (q));

        if (!r.empty ())
        {
          cout << lower << '-' << (upper - 1) << ':' << endl;
          print (r);
        }
      }

      t.commit ();
    }

    // Query that shows how to use the in() function.
    //
    {
      transaction t (db->begin ());

      result r (
        db->query<person> (
          query::last.in ("Smith", "Johnson", "Jockson")));

      print (r);

      t.commit ();
    }

    // The same query but using the in_range() function.
    //
    {
      vector<string> names;

      names.push_back ("Smith");
      names.push_back ("Johnson");
      names.push_back ("Jockson");

      transaction t (db->begin ());

      result r (
        db->query<person> (
          query::last.in_range (names.begin (), names.end ())));

      print (r);

      t.commit ();
    }

    // Query that shows how to test for NULL values using the
    // is_null()/is_not_null() functions.
    //
    {
      transaction t (db->begin ());
      result r (db->query<person> (query::middle.is_not_null ()));
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
