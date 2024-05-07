// file      : c++11/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/session.hxx>
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

    // Create a few persistent objects.
    //
    {
      pension_fund bf ("Bright Future");

      // Simple Tech Ltd.
      //
      {
        shared_ptr<employer> er (new employer ("Simple Tech Ltd"));

        shared_ptr<employee> john (new employee ("John", "Doe", er));
        shared_ptr<employee> jane (new employee ("Jane", "Doe", er));

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

        bf.members ().push_back (lazy_shared_ptr<employee> (*db, john));
      }

      // Complex Systems Inc.
      //
      {
        shared_ptr<employer> er (new employer ("Complex Systems Inc"));

        shared_ptr<employee> john (new employee ("John", "Smith", er));
        shared_ptr<employee> jane (new employee ("Jane", "Smith", er));

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

        bf.members ().push_back (lazy_shared_ptr<employee> (*db, jane));
      }

      transaction t (db->begin ());
      db->persist (bf);
      t.commit ();
    }

    // Load the Bright Future pension fund and print its members.
    //
    {
      session s;
      transaction t (db->begin ());

      unique_ptr<pension_fund> bf (db->load<pension_fund> ("Bright Future"));

      for (auto i (bf->members ().begin ()); i != bf->members ().end (); ++i)
      {
        lazy_shared_ptr<employee>& p (*i);
        p.load ();

        cout << p->first () << ' ' << p->last () << ", " <<
          p->employer ()->name () << endl;
      }

      // Alternative implementation using range-based for-loop.
      //
      /*
      for (lazy_shared_ptr<employee>& p: bf->members ())
      {
        p.load ();
        cout << p->first () << ' ' << p->last () << ", " <<
          p->employer ()->name () << endl;
      }
      */

      cout << endl;
    }

    // Load Simple Tech Ltd and print its employees.
    //
    {
      session s;
      transaction t (db->begin ());

      shared_ptr<employer> st (db->load<employer> ("Simple Tech Ltd"));

      for (auto i (st->employees ().begin ());
           i != st->employees ().end ();
           ++i)
      {
        lazy_weak_ptr<employee>& lwp (*i);
        shared_ptr<employee> p (lwp.load ()); // Load and lock.

        cout << p->first () << ' ' << p->last () << endl
             << "  employer: " << p->employer ()->name () << endl;

        for (auto j (p->emails ().begin ()); j != p->emails ().end (); ++j)
        {
          cout << "  email: " << *j << endl;
        }

        cout << endl;
      }

      // Alternative implementation using range-based for-loop.
      //
      /*
      for (lazy_weak_ptr<employee>& lwp: st->employees ())
      {
        shared_ptr<employee> p (lwp.load ()); // Load and lock.

        cout << p->first () << ' ' << p->last () << endl
             << "  employer: " << p->employer ()->name () << endl;

        for (const string& e: p->emails ())
          cout << "  email: " << e << endl;

        cout << endl;
      }
      */

      t.commit ();
    }


    // Search for Complex Systems Inc employees with the John first name.
    //
    {
      typedef odb::query<employee> query;
      typedef odb::result<employee> result;

      session s;
      transaction t (db->begin ());

      result r (db->query<employee> (
                  query::employer->name == "Complex Systems Inc" &&
                  query::first == "John"));

      for (auto i (r.begin ()); i != r.end (); ++i)
        cout << i->first () << ' ' << i->last () << endl;

      // Alternative implementation using range-based for-loop.
      //
      /*
      for (const employee& e: r)
        cout << e.first () << ' ' << e.last () << endl;
      */

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
