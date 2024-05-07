// file      : inheritance/polymorphism/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include "database.hxx" // create_database

#include "employee.hxx"
#include "employee-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    unsigned long id1, id2;

    // Add a few employee and contractor objects to the database.
    //
    {
      unique_ptr<person> p1 (new employee ("John", "Doe", true));
      unique_ptr<person> p2 (new contractor ("Jane", "Doe", "jane@doe.com"));

      transaction t (db->begin ());
      id1 = db->persist (*p1); // Stores employee.
      id2 = db->persist (*p2); // Stores contractor.
      t.commit ();
    }

    // Load polymorphic objects given their object ids.
    //
    {
      transaction t (db->begin ());
      unique_ptr<person> p1 (db->load<person> (id1)); // Loads employee.
      unique_ptr<person> p2 (db->load<person> (id2)); // Loads contractor.
      t.commit ();

      p1->print ();
      p2->print ();
    }

    // Make John Doe a permanent employee.
    //
    {
      transaction t (db->begin ());

      unique_ptr<employee> e (db->load<employee> (id1));
      e->temporary (false);
      person& p (*e);
      db->update (p); // Updates employee.

      t.commit ();
    }

    // Query all the person objects that have Doe as the last name.
    //
    {
      typedef odb::query<person> query;
      typedef odb::result<person> result;

      transaction t (db->begin ());

      result r (db->query<person> (query::last == "Doe"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        // We can check the discriminator before loading the object.
        //
        string d (i.discriminator ());
        cout << "discriminator: " << d << endl;

        i->print (); // Can be employee or contractor.
      }

      t.commit ();
    }

    // Erase the objects from the database.
    //
    {
      transaction t (db->begin ());
      unique_ptr<person> p (db->load<person> (id1)); // Loads employee.
      db->erase (*p);                                // Erases employee.
      db->erase<person> (id2);                       // Erases contractor.
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
