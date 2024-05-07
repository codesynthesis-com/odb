// file      : view/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr, std::shared_ptr
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

    // Create the legacy employee_extra table.
    //
    {
      // First try to drop the table if it exists.
      //
      {
        transaction t (db->begin ());
        try
        {
#ifndef DATABASE_ORACLE
          db->execute ("DROP TABLE view_employee_extra");
#else
          db->execute ("DROP TABLE \"view_employee_extra\"");
#endif
          t.commit ();
        }
        catch (const odb::exception&)
        {
        }
      }

      {
        transaction t (db->begin ());

#ifndef DATABASE_ORACLE
        db->execute (
          "CREATE TABLE view_employee_extra ("
          "employee_id INTEGER NOT NULL,"
          "vacation_days INTEGER NOT NULL,"
          "previous_employer_id INTEGER)");
#else
        db->execute (
          "CREATE TABLE \"view_employee_extra\" ("
          "\"employee_id\" INTEGER NOT NULL,"
          "\"vacation_days\" INTEGER NOT NULL,"
          "\"previous_employer_id\" INTEGER)");
#endif

        t.commit ();
      }
    }

    // Create a few persistent objects.
    //
    {
      shared_ptr<country> ca (new country ("CA", "Canada"));
      shared_ptr<country> za (new country ("ZA", "South Africa"));
      shared_ptr<country> us (new country ("US", "United States"));
      shared_ptr<country> se (new country ("SE", "Sweden"));

      shared_ptr<employer> st (new employer (1, "Simple Tech Ltd"));
      shared_ptr<employer> cs (new employer (2, "Complex Systems Inc"));

      shared_ptr<employee> e1 (
        new employee (1, "John", "Doe", 29, ca, ca, st));

      shared_ptr<employee> e2 (
        new employee (2, "Jane", "Doe", 30, za, us, cs));

      shared_ptr<employee> e3 (
        new employee (3, "Joe", "Dirt", 31, us, za, st));

      shared_ptr<employee> e4 (
        new employee (4, "Johan", "Johansen", 32, se, se, cs));

      transaction t (db->begin ());

      db->persist (ca);
      db->persist (za);
      db->persist (us);
      db->persist (se);

      db->persist (st);
      db->persist (cs);

      db->persist (e1);
      db->persist (e2);
      db->persist (e3);
      db->persist (e4);

      // Populate the legacy table.
      //
#ifndef DATABASE_ORACLE
      db->execute ("INSERT INTO view_employee_extra ("
                   "employee_id, vacation_days, previous_employer_id) "
                   "VALUES (1, 5, 2)");

      db->execute ("INSERT INTO view_employee_extra ("
                   "employee_id, vacation_days, previous_employer_id) "
                   "VALUES (2, 10, NULL)");

      db->execute ("INSERT INTO view_employee_extra ("
                   "employee_id, vacation_days, previous_employer_id) "
                   "VALUES (3, 0, NULL)");

      db->execute ("INSERT INTO view_employee_extra ("
                   "employee_id, vacation_days, previous_employer_id) "
                   "VALUES (4, 15, 1)");
#else
      db->execute ("INSERT INTO \"view_employee_extra\" ("
                   "\"employee_id\", "
                   "\"vacation_days\", "
                   "\"previous_employer_id\") "
                   "VALUES (1, 5, 2)");

      db->execute ("INSERT INTO \"view_employee_extra\" ("
                   "\"employee_id\", "
                   "\"vacation_days\", "
                   "\"previous_employer_id\") "
                   "VALUES (2, 10, NULL)");

      db->execute ("INSERT INTO \"view_employee_extra\" ("
                   "\"employee_id\", "
                   "\"vacation_days\", "
                   "\"previous_employer_id\") "
                   "VALUES (3, 0, NULL)");

      db->execute ("INSERT INTO \"view_employee_extra\" ("
                   "\"employee_id\", "
                   "\"vacation_days\", "
                   "\"previous_employer_id\") "
                   "VALUES (4, 15, 1)");
#endif

      t.commit ();
    }

    // Load names of the employees that are under 31 using the employee_name
    // view.
    //
    {
      typedef odb::query<employee_name> query;
      typedef odb::result<employee_name> result;

      transaction t (db->begin ());

      result r (db->query<employee_name> (query::age < 31));

      cout << "Employees under 31" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->first << " " << i->last << endl;

      cout << endl;

      t.commit ();
    }

    // Count the number of employees which has the Doe last name using the
    // employee_count view.
    //
    {
      transaction t (db->begin ());

      // Result of an aggregate query contains only one element so let's
      // use the query_value() shortcut.
      //
      employee_count ec (
        db->query_value<employee_count> (
          query<employee_count>::last == "Doe"));

      cout << ec.count << " employees with the Doe last name" << endl
           << endl;

      t.commit ();
    }

    // Load the employee-employer information for all the employees with the
    // Doe last name using the employee_employer view.
    //
    {
      typedef odb::query<employee_employer> query;
      typedef odb::result<employee_employer> result;

      transaction t (db->begin ());

      // Note that we need to add the object name after query::.
      //
      result r (db->query<employee_employer> (query::employee::last == "Doe"));

      cout << "Employees with the Doe last name" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->first << " " << i->last <<  " "
             << i->employer_name << endl;

      cout << endl;

      t.commit ();
    }

    // Calculate min/max employee ages for each employer.
    //
    {
      typedef odb::result<employer_age> result;

      transaction t (db->begin ());

      result r (db->query<employer_age> ());

      // Some other interesting queries to try:
      //
      // This one restricts the calculation to a specific employer:
      //
      // typedef odb::query<employer_age> query;
      //
      // result r (db->query<employer_age> (
      //             query::employer::name == "Simple Tech Ltd"));
      //
      // And this one filters the employees based on certain criteria.
      //
      // result r (db->query<employer_age> (
      //             query::employee::last == "Doe"));
      //

      cout << "Min/max employee ages" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->employer_name << " "
             << i->min_age << '/' << i->max_age << endl;

      cout << endl;

      t.commit ();
    }

    // Load the country information for employees with different residence
    // and nationality.
    //
    {
      typedef odb::query<employee_country> query;
      typedef odb::result<employee_country> result;

      transaction t (db->begin ());

      // Note that we use the alias given in the db object pragma after
      // query::.
      //
      result r (db->query<employee_country> (
                  query::res_country::name != query::nat_country::name));

      cout << "Employees residing outside of country of nationality" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->first << " " << i->last << " "
             << i->res_country_name << " " << i->nat_country_name << endl;

      cout << endl;

      t.commit ();
    }

    // The same but using the object loading view.
    //
    {
      typedef odb::query<employee_country_objects> query;
      typedef odb::result<employee_country_objects> result;

      transaction t (db->begin ());

      // We have to use a session in order for the object pointers
      // in our view and object pointers inside objects that we load
      // to point to the same instances, where appropriate.
      //
      session s;

      result r (db->query<employee_country_objects> (
                  query::res::name == query::nat::name));

      cout << "Employees residing inside the country of nationality" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        assert (i->e->nationality () == i->nat);
        assert (i->e->residence () == i->res);

        const employee& e (*i->e);
        const country& r (*i->res);
        const country& n (*i->nat);

        cout << "  " << e.first () << " " << e.last () << " "
             << r.name () << " " << n.name () << endl;
      }

      cout << endl;

      t.commit ();
    }

    // Get the list of employers that have any employees.
    //
    {
      typedef odb::result<employer_with_employees> result;

      shared_ptr<employer> es (new employer (3, "Empty Shell LLC"));

      transaction t (db->begin ());
      db->persist (es);

      result r (db->query<employer_with_employees> ());

      cout << "Employers with employees" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->e->name () << endl;

      cout << endl;

      db->erase (es);
      t.commit ();
    }

    // Get the list of employees that have accumulated vacation days.
    //
    {
      typedef odb::result<employee_vacation> result;

      transaction t (db->begin ());

      // With native views we have to use the native SQL query syntax.
      //
#ifndef DATABASE_ORACLE
      result r (db->query<employee_vacation> ("vacation_days <> 0"));
#else
      result r (db->query<employee_vacation> ("\"vacation_days\" <> 0"));
#endif

      cout << "Employees with accumulated vacation days" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->id << " " << i->days << endl;

      cout << endl;

      t.commit ();
    }

    // Get the list of employees that have accumulated vacation days, this
    // time using the improved employee_vacation2 view.
    //
    {
      typedef odb::result<employee_vacation2> result;

      transaction t (db->begin ());

#ifndef DATABASE_ORACLE
      result r (db->query<employee_vacation2> ("vacation_days <> 0"));
#else
      result r (db->query<employee_vacation2> ("\"vacation_days\" <> 0"));
#endif

      cout << "Employees with accumulated vacation days (take 2)" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << "  " << i->first << " " << i->last << " " << i->days << endl;

      cout << endl;

      t.commit ();
    }

    // Show the previous employers using the employee_prev_employer view.
    //
    {
      typedef odb::result<employee_prev_employer> result;

      transaction t (db->begin ());

      result r (db->query<employee_prev_employer> ());

      cout << "Previous employees" << endl;

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        const nullable<string>& pe (i->prev_employer_name);

        cout << "  " << i->first << " " << i->last << " "
             << (pe.null () ? string ("N/A") : *pe) << endl;
      }

      cout << endl;

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
