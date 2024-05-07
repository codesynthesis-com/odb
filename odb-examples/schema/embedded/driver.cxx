// file      : schema/embedded/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>

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
    typedef odb::query<person> query;
    typedef odb::result<person> result;

    unique_ptr<database> db (create_database (argc, argv));

    // Create the database schema.
    //
    {
      transaction t (db->begin ());
      schema_catalog::create_schema (*db);
      t.commit ();
    }

    // The following alternative version only creates the schema if it
    // hasn't already been created. To detect the existence of the schema
    // this version tries to query the database for a person object. If
    // the corresponding table does not exist, then an exceptions will be
    // thrown in which case we proceed to creating the schema.
    //
    /*
    {
      transaction t (db->begin ());

      try
      {
        db->query<person> (false);
      }
      catch (const odb::exception& e)
      {
        schema_catalog::create_schema (*db);
      }

      t.commit ();
    }
    */

    // Create a few persistent person objects.
    //
    {
      person john ("John", "Doe", 33);
      person jane ("Jane", "Doe", 32);
      person joe ("Joe", "Dirt", 30);

      transaction t (db->begin ());

      db->persist (john);
      db->persist (jane);
      db->persist (joe);

      t.commit ();
    }

    // Print those over 30.
    //
    {
      transaction t (db->begin ());

      result r (db->query<person> (query::age > 30));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        cout << i->first () << " " << i->last () << endl;
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
