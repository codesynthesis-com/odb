// file      : boost/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::auto_ptr
#include <iostream>

#include <boost/uuid/uuid_io.hpp>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

#include "database.hxx" // create_database

#include "employee.hxx"
#include "employee-odb.hxx"

using namespace std;
using namespace boost::gregorian;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  using boost::shared_ptr;

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Create a few persistent objects.
    //
    {
      // Simple Tech Ltd.
      //
      {
        shared_ptr<employer> er (new employer ("Simple Tech Ltd"));

        shared_ptr<employee> john (
          new employee ("John", "Doe", date (1975, Jan, 1), er));

        shared_ptr<employee> jane (
          new employee ("Jane", "Q", "Doe", date (1976, Feb, 2), er));

        john->emails ().insert ("john_d@example.com");
        john->emails ().insert ("john.doe@example.com");
        jane->emails ().insert ("jane_d@example.com");
        jane->emails ().insert ("jane.doe@example.com");

        // Set the inverse side of the employee-employer relationship.
        //
        er->employees ().push_back (john);
        er->employees ().push_back (jane);

        transaction t (db->begin ());

        db->persist (er);

        db->persist (john);
        db->persist (jane);

        t.commit ();
      }

      // Complex Systems Inc.
      //
      {
        shared_ptr<employer> er (new employer ("Complex Systems Inc"));

        shared_ptr<employee> john (
          new employee ("John", "Z", "Smith", date (1977, Mar, 3), er));

        shared_ptr<employee> jane (
          new employee ("Jane", "Smith", date (1978, Apr, 4), er));

        john->emails ().insert ("john_s@example.com");
        john->emails ().insert ("john.smith@example.com");
        jane->emails ().insert ("jane_s@example.com");
        jane->emails ().insert ("jane.smith@example.com");

        // Set the inverse side of the employee-employer relationship.
        //
        er->employees ().push_back (john);
        er->employees ().push_back (jane);

        transaction t (db->begin ());

        db->persist (er);

        db->persist (john);
        db->persist (jane);

        t.commit ();
      }
    }


    // Load Simple Tech Ltd and print its employees.
    //
    {
      session s;
      transaction t (db->begin ());

      shared_ptr<employer> stl (db->load<employer> ("Simple Tech Ltd"));

      employees& es (stl->employees ());

      for (employees::iterator i (es.begin ()); i != es.end (); ++i)
      {
        lazy_weak_ptr<employee>& lwp (*i);
        shared_ptr<employee> p (lwp.load ()); // Load and lock.

        cout << p->first () << " ";

        if (p->middle ())
          cout << *p->middle () << " ";

        cout << p->last () << endl;

        cout << "  born: " << p->born () << endl
             << "  employer: " << p->employer ()->name () << endl;

        for (emails::const_iterator j (p->emails ().begin ());
             j != p->emails ().end (); ++j)
        {
          cout << "  email: " << *j << endl;
        }

        cout << "  id: {" << p->id () << '}' << endl
             << endl;
      }

      t.commit ();
    }

    typedef odb::query<employee> query;
    typedef odb::result<employee> result;

    // Search for Complex Systems Inc employees that were born before
    // April 1978.
    //
    {
      session s;
      transaction t (db->begin ());

      result r (db->query<employee> (
                  query::employer->name == "Complex Systems Inc" &&
                  query::born < date (1978, Apr, 1)));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->first () << " " << i->last () << " " << i->born () << endl;

      cout << endl;
      t.commit ();
    }

    // Search for all the employees that don't have a middle name.
    //
    {
      session s;
      transaction t (db->begin ());

      result r (db->query<employee> (query::middle.is_null ()));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->first () << " " << i->last () << endl;

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
