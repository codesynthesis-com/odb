// file      : section/driver.cxx
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

    unsigned long id;

    // Create a persistent person object.
    //
    {
      person joe ("Joe", "Dirt");
      joe.bio ("Joe Dirt is a very strange man indeed...");
      joe.nicknames ().push_back ("JD");

      // At this point the state of a section is undefined since the
      // object is transient.

      transaction t (db->begin ());
      id = db->persist (joe);
      t.commit ();

      // Now, since the object is persistent, the state of the section
      // is loaded and unchanged.
    }

    // Load the object and print what we've got. Then update the bio.
    //
    {
      transaction t (db->begin ());
      unique_ptr<person> p (db->load<person> (id));

      // Now, while the object is loaded, the section is not. If (and
      // when) we need to access the section data members, we have to
      // load them explicitly.

      cout << p->first () << " " << p->last () << endl;

      db->load (*p, p->extras_section ());

      // Now the section is loaded and unchanged.

      cout << "  " << p->bio () << endl
           << "  " << p->nicknames ().size () << " nickname(s)" << endl;

      // Update the bio.
      //
      p->bio ("Joe Dirt is a very clean man indeed...");

      // Now, the section is marked changed by the bio() modifier.
      // Because of that, the following object update will also
      // update the section.
      //
      db->update (*p);

      t.commit ();
    }

    // Add a new nickname.
    //
    {
      transaction t (db->begin ());

      unique_ptr<person> p (db->load<person> (id));
      db->load (*p, p->extras_section ());

      // Add a new nickname. The nicknames container is now marked
      // changed.
      //
      p->nicknames ().push_back ("Squeaky Clean");

      // While the section hasn't been marked as changed, a change-
      // tracking container that belongs to it has been. As a result,
      // the following object update call will automatically mark the
      // section as changed and update its state in the database.
      //
      db->update (*p);

      t.commit ();
    }

    // We can also update just the section if we know the rest of
    // the object hasn't changed.
    //
    {
      transaction t (db->begin ());

      unique_ptr<person> p (db->load<person> (id));
      db->load (*p, p->extras_section ());

      p->nicknames ().push_back ("Dirty Joe");
      db->update (*p, p->extras_section ());

      t.commit ();
    }

    // Load and print the updated object.
    //
    {
      transaction t (db->begin ());
      unique_ptr<person> p (db->load<person> (id));
      db->load (*p, p->extras_section ());
      t.commit ();

      cout << p->first () << " " << p->last () << endl
           << "  " << p->bio () << endl
           << "  " << p->nicknames ().size () << " nickname(s)" << endl;
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
