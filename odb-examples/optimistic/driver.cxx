// file      : optimistic/driver.cxx
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

    // Persist the object.
    //
    // At this point the initial version (1) is assigned.
    //
    unsigned long id;
    {
      person p ("John", "Doe", 21);

      transaction t (db->begin ());
      id = db->persist (p);
      t.commit ();

      cout << "version after persist: " << p.version () << endl;
    }

    // Process 1: load the object.
    //
    person p1;
    {
      transaction t (db->begin ());
      db->load (id, p1);
      t.commit ();

      cout << "process 1 version after load: " << p1.version () << endl;
    }

    // Process 2: load the object.
    //
    person p2;
    {
      transaction t (db->begin ());
      db->load (id, p2);
      t.commit ();

      cout << "process 2 version after load: " << p2.version () << endl;
    }

    // Process 1: update the object.
    //
    // At this point the version is incremented and becomes 2.
    //
    {
      p1.age (20); // Correct the wrong age.

      transaction t (db->begin ());
      db->update (p1);
      t.commit ();

      cout << "process 1 version after update: " << p1.version () << endl;
    }

    // Process 2: update the object.
    //
    // Since the object version in this process is 1 while in the database
    // it is 2, this operation will fail.
    //
    {
      p2.age (p2.age () + 1); // Increment the age.

      transaction t (db->begin ());

      try
      {
        db->update (p2);
      }
      catch (const object_changed&)
      {
        cout << "process 2 version is out of date: " << p2.version () << endl;

        // Reload the object and retry the operation. Note that the second
        // update call cannot throw object_changed since we reloaded the
        // object and are trying to update it in a single transaction.
        //
        db->reload (p2);

        cout << "process 2 version after reload: " << p2.version () << endl;

        p2.age (p2.age () + 1);
        db->update (p2);
      }

      t.commit ();

      cout << "process 2 version after update: " << p2.version () << endl;
      cout << "final age value: " << p2.age () << endl;
    }

    // Process 1: delete the object if the person is younger than 21.
    //
    // Since the object version in this process is 2 while in the database
    // it is 3, this operation will fail. Note that this will only hold
    // true if we are deleting the object by passing an object instance
    // to the erase() function. If instead we pass object id, then the
    // object will be deleted regardless of the version.
    //
    if (p1.age () < 21)
    {
      transaction t (db->begin ());

      try
      {
        db->erase (p1);
        // db->erase (id); // Never throws object_changed.
      }
      catch (const object_changed&)
      {
        cout << "process 1 version is out of date: " << p1.version () << endl;

        // Reload the object and retry the operation. Similar to update, the
        // second erase call cannot throw object_changed since we reloaded
        // the object and are trying to erase it in a single transaction.
        //
        db->reload (p1);

        cout << "process 1 version after reload: " << p2.version () << endl;

        if (p1.age () < 21)
        {
          db->erase (p1);
          cout << "object deleted" << endl;
        }
        else
          cout << "object not deleted" << endl;
      }

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
