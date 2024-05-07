// file      : container/driver.cxx
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

void
print (const person& p)
{
  cout << p.first () << " " << p.last () << endl;

  // Print nicknames.
  //
  for (names::const_iterator i (p.nicknames ().begin ());
       i != p.nicknames ().end (); ++i)
  {
    cout << "  nickname: " << *i << endl;
  }

  // Print emails.
  //
  for (emails::const_iterator i (p.emails ().begin ());
       i != p.emails ().end (); ++i)
  {
    cout << "  email: " << *i << endl;
  }

  // Print weights.
  //
  for (age_weight_map::const_iterator i (p.age_weight ().begin ());
       i != p.age_weight ().end (); ++i)
  {
    cout << "  weight at " << i->first << ": " << i->second << endl;
  }
}

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    unsigned long id;

    // Create a persistent person object.
    //
    {
      person joe ("Joe", "Dirt");

      joe.nicknames ().push_back ("JD");
      joe.nicknames ().push_back ("Squeaky");

      joe.emails ().insert ("joe@example.com");
      joe.emails ().insert ("joe.dirt@example.com");

      joe.age_weight ()[5] = 15.6F;
      joe.age_weight ()[10] = 23.3F;
      joe.age_weight ()[15] = 29.8F;

      transaction t (db->begin ());
      id = db->persist (joe);
      t.commit ();
    }

    // Load the object and print what we've got. Then change some
    // information and update it in the database.
    //
    {
      transaction t1 (db->begin ());
      unique_ptr<person> j (db->load<person> (id));
      t1.commit ();

      print (*j);

      // Ann another nickname.
      //
      j->nicknames ().push_back ("Cleaner");

      // The joe@example.com email is no longer working.
      //
      j->emails ().erase ("joe@example.com");

      // Update a weight sample.
      //
      j->age_weight ()[15] = 28.8F;

      transaction t2 (db->begin ());
      db->update (*j);
      t2.commit ();
    }

    // Load and print the updated object.
    //
    {
      transaction t (db->begin ());
      unique_ptr<person> j (db->load<person> (id));
      t.commit ();

      print (*j);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
