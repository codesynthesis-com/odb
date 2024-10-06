// file      : container-nested/driver.cxx
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

  // Print trips.
  //
  for (trips::const_iterator i (p.trips ().begin ());
       i != p.trips ().end (); ++i)
  {
    cout << "  trip:";

    for (trip::const_iterator j (i->begin ()); j != i->end (); ++j)
    {
      if (j != i->begin ())
        cout << ',';

      cout << ' ' << j->name << ": " << j->days;
    }

    cout << endl;
  }

  // Print emergency contacts.
  //
  for (emergency_contacts::const_iterator i (p.emergency_contacts ().begin ());
       i != p.emergency_contacts ().end (); ++i)
  {
    cout << "  emergency contact: " << i->first_name << ' ' << i->last_name;

    for (phone_numbers::const_iterator j (i->numbers.begin ());
         j != i->numbers.end (); ++j)
    {
      if (j != i->numbers.begin ())
        cout << ',';

      cout << ' ' << *j;
    }

    cout << endl;
  }

  // Print age weights.
  //
  for (age_weights_map::const_iterator i (p.age_weights ().begin ());
       i != p.age_weights ().end (); ++i)
  {
    cout << "  age weights: " << i->first << ":";

    for (weights::const_iterator j (i->second.begin ());
         j != i->second.end (); ++j)
    {
      if (j != i->second.begin ())
        cout << ',';

      cout << ' ' << *j;
    }

    cout << endl;
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

      joe.trips ().push_back (trip {country_stay {"Colombia", 1},
                                    country_stay {"Peru", 2}});

      joe.trips ().push_back (trip {country_stay {"France", 3},
                                    country_stay {"Italy", 4}});

      joe.emergency_contacts ().push_back ({
        emergency_contact {
          "Jane", "Doe", phone_numbers {"335-53-81", "198-23-32"}}});

      joe.emergency_contacts ().push_back ({
        emergency_contact {
          "John", "Smith", phone_numbers {"137-43-06"}}});

      joe.age_weights ()[40] = weights {70, 75};
      joe.age_weights ()[50] = weights {75, 80};

      transaction t (db->begin ());
      id = db->persist (joe);
      t.commit ();
    }

    // Load the object and print what we've got. Then change some information
    // and update it in the database.
    //
    {
      transaction t1 (db->begin ());
      unique_ptr<person> j (db->load<person> (id));
      t1.commit ();

      print (*j);

      // Add another trip.
      //
      j->trips ().push_back (trip {country_stay {"China", 6},
                                   country_stay {"Japan", 7}});

      // Update an existing trip.
      //
      j->trips ()[1][1].days = 5;

      // Add another emergency contact.
      //
      j->emergency_contacts ().push_back ({
        emergency_contact {
          "Jack", "Johnson", phone_numbers {"258-23-00", "712-80-39"}}});

      // Update an existing emergency contact.
      //
      j->emergency_contacts ()[0].numbers[1] = "198-23-33";

      // Add another age weights.
      //
      j->age_weights ()[60] = weights {80, 75};

      // Update an existing age weight.
      //
      j->age_weights ()[50][1] = 79;

      // Erase existing age weights.
      //
      j->age_weights ().erase (40);

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
