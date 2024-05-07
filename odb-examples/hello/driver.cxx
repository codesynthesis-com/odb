// file      : hello/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include "database.hxx" // create_database

#include "person.hxx"
#include "person-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    unsigned long john_id, joe_id;

    // Create a few persistent person objects.
    //
    {
      person john ("John", "Doe", 33);
      person jane ("Jane", "Doe", 32);
      person joe ("Joe", "Dirt", 30);

      transaction t (db->begin ());

      // Make objects persistent and save their ids for later use.
      //
      john_id = db->persist (john);
      db->persist (jane);
      joe_id = db->persist (joe);

      t.commit ();
    }

    typedef odb::query<person> query;
    typedef odb::result<person> result;

    // Say hello to those over 30.
    //
    {
      transaction t (db->begin ());

      result r (db->query<person> (query::age > 30));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        cout << "Hello, " << i->first () << " " << i->last () << "!" << endl;
      }

      t.commit ();
    }

    // Joe Dirt just had a birthday, so update his age.
    //
    {
      transaction t (db->begin ());

      unique_ptr<person> joe (db->load<person> (joe_id));
      joe->age (joe->age () + 1);
      db->update (*joe);

      t.commit ();
    }

    // Alternative implementation without using the id.
    //
    /*
    {
      transaction t (db->begin ());

      // Here we know that there can be only one Joe Dirt in our
      // database so we use the query_one() shortcut instead of
      // manually iterating over the result returned by query().
      //
      unique_ptr<person> joe (
        db->query_one<person> (query::first == "Joe" &&
                               query::last == "Dirt"));

      if (joe.get () != 0)
      {
        joe->age (joe->age () + 1);
        db->update (*joe);
      }

      t.commit ();
    }
    */

    // Print some statistics about all the people in our database.
    //
    {
      transaction t (db->begin ());

      // The result of this (aggregate) query always has exactly one element
      // so use the query_value() shortcut.
      //
      person_stat ps (db->query_value<person_stat> ());

      cout << endl
           << "count  : " << ps.count << endl
           << "min age: " << ps.min_age << endl
           << "max age: " << ps.max_age << endl;

      t.commit ();
    }

    // John Doe is no longer in our database.
    //
    {
      transaction t (db->begin ());
      db->erase<person> (john_id);
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
