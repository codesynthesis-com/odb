// file      : composite/driver.cxx
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

    // Create a person object.
    //
    email_address id;
    {
      person p ("joe@example.com",
                "Joe",
                "Dirt",
                "Mr",
                phone_numbers ("555 5555", "666 6666"));

      transaction t (db->begin ());
      id = db->persist (p);
      t.commit ();
    }

    // Update the extra name information.
    //
    {
      transaction t (db->begin ());

      unique_ptr<person> joe (db->load<person> (id));
      name_extras& ne (joe->name ().extras ());
      ne.nickname ("Squeaky");
      ne.aliases ().push_back (basic_name ("Anthony", "Clean"));

      db->update (*joe);

      t.commit ();
    }

    // Print the name and phone numbers.
    //
    {
      transaction t (db->begin ());
      unique_ptr<person> joe (db->load<person> (id));
      t.commit ();

      name& n (joe->name ());

      cout << n.title () << " " << n.first () << " " << n.last () << " "
           << '<' << joe->email ().address () << '>' << endl;

      name_extras& ne (n.extras ());

      if (!ne.nickname ().empty ())
        cout << "  nickname: " << ne.nickname () << endl;

      for (basic_names::iterator i (ne.aliases ().begin ());
           i != ne.aliases ().end ();
           ++i)
      {
        cout << "  alias: " << i->first () << " " << i->last () << endl;
      }

      cout << "  phone 1: " << joe->phone ().first << endl;
      cout << "  phone 2: " << joe->phone ().second << endl;
    }

    // Query the database for a person object.
    //
    {
      typedef odb::query<person> query;

      transaction t (db->begin ());

      unique_ptr<person> p (
        db->query_one<person> (
          query::name.extras.nickname == "Squeaky"));

      if (p.get () != 0)
      {
        name& n (p->name ());
        cout << n.title () << " " << n.first () << " " << n.last () << endl;
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
