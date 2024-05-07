// file      : inheritance/reuse/driver.cxx
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

    // Add a few employees and contractors to the database.
    //
    {
      permanent_employee p1 ("John", "Doe");
      permanent_employee p2 ("Jane", "Doe");

      temporary_employee t1 ("John", "Smith", 6);
      temporary_employee t2 ("Jane", "Smith", 12);

      contractor c1 ("Joe", "Doe", "j.doe@example.com");
      contractor c2 ("Joe", "Smith", "j.smith@example.com");

      transaction t (db->begin ());
      db->persist (p1);
      db->persist (p2);
      db->persist (t1);
      db->persist (t2);
      db->persist (c1);
      db->persist (c2);
      t.commit ();
    }

    // Lookup a contractor based on the email address.
    //
    {
      transaction t (db->begin ());
      unique_ptr<contractor> c (db->load<contractor> ("j.smith@example.com"));
      t.commit ();

      cout << c->first () << " " << c->last () << " " << c->email () << endl;
    }

    // Query for temporary employees that have John as the first name.
    //
    {
      typedef odb::query<temporary_employee> query;
      typedef odb::result<temporary_employee> result;

      transaction t (db->begin ());

      result r (db->query<temporary_employee> (query::first == "John"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        cout << i->first () << " " << i->last () << " "
             << i->duration () << " months" << endl;
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
