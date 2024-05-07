// file      : schema/custom/driver.cxx
// copyright : not copyrighted - public domain

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/database.hxx>
#include <odb/connection.hxx>
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

    // Create the database schema.
    //
#if defined(DATABASE_MYSQL)  || \
    defined(DATABASE_SQLITE) || \
    defined(DATABASE_MSSQL)
    {

      // Due to bugs in SQLite foreign key support for DDL statements,
      // we need to temporarily disable foreign keys.
      //
      connection_ptr c (db->connection ());

#ifdef DATABASE_SQLITE
      c->execute ("PRAGMA foreign_keys=OFF");
#endif

      transaction t (c->begin ());

      // Try to drop the tables if they exist and ignore the error
      // if they don't.
      //
      try
      {
        db->execute ("DROP TABLE EmployeeDegree");
        db->execute ("DROP TABLE Employee");
        db->execute ("DROP TABLE Employer");
      }
      catch (const odb::exception&)
      {
      }

      db->execute (
        "CREATE TABLE Employer ("
        "name VARCHAR (255) NOT NULL PRIMARY KEY)");

      db->execute (
        "CREATE TABLE Employee ("
        "ssn INTEGER NOT NULL PRIMARY KEY,"
        "first_name VARCHAR (255) NOT NULL,"
        "last_name VARCHAR (255) NOT NULL,"
        "employer VARCHAR (255) NOT NULL REFERENCES Employer (name))");

      db->execute (
        "CREATE TABLE EmployeeDegree ("
        "ssn INTEGER NOT NULL REFERENCES Employee (ssn),"
        "degree VARCHAR (255) NOT NULL)");

      t.commit ();

#ifdef DATABASE_SQLITE
      c->execute ("PRAGMA foreign_keys=ON");
#endif
    }
#elif defined(DATABASE_PGSQL)
    {
      // PostgreSQL-specific SQL.
      //
      transaction t (db->begin ());

      db->execute ("DROP TABLE IF EXISTS \"Employer\" CASCADE");
      db->execute ("DROP TABLE IF EXISTS \"Employee\" CASCADE");
      db->execute ("DROP TABLE IF EXISTS \"EmployeeDegree\" CASCADE");

      db->execute (
        "CREATE TABLE \"Employer\" ("
        "name VARCHAR (255) NOT NULL PRIMARY KEY)");

      db->execute (
        "CREATE TABLE \"Employee\" ("
        "ssn INTEGER NOT NULL PRIMARY KEY,"
        "first_name VARCHAR (255) NOT NULL,"
        "last_name VARCHAR (255) NOT NULL,"
        "employer VARCHAR (255) NOT NULL)");

      db->execute (
        "CREATE TABLE \"EmployeeDegree\" ("
        "ssn INTEGER NOT NULL,"
        "degree VARCHAR (255) NOT NULL)");

      db->execute (
        "ALTER TABLE \"Employee\" "
        "ADD FOREIGN KEY (employer) "
        "REFERENCES \"Employer\" "
        "INITIALLY DEFERRED");

      db->execute (
        "ALTER TABLE \"EmployeeDegree\" "
        "ADD FOREIGN KEY (ssn) "
        "REFERENCES \"Employee\" "
        "INITIALLY DEFERRED");

      t.commit ();
    }
#elif defined(DATABASE_ORACLE)
    {
      // Oracle-specific PL/SQL.
      //
      transaction t (db->begin ());

      db->execute ("BEGIN "
                   "  EXECUTE IMMEDIATE "
                   "    'DROP TABLE \"Employer\" CASCADE CONSTRAINTS';"
                   "  EXCEPTION "
                   "    WHEN OTHERS THEN "
                   "      IF SQLCODE != -942 THEN RAISE; END IF;"
                   "END;");

      db->execute ("BEGIN "
                   "  EXECUTE IMMEDIATE "
                   "    'DROP TABLE \"Employee\" CASCADE CONSTRAINTS';"
                   "  EXCEPTION "
                   "    WHEN OTHERS THEN "
                   "      IF SQLCODE != -942 THEN RAISE; END IF;"
                   "END;");

      db->execute ("BEGIN "
                   "  EXECUTE IMMEDIATE 'DROP TABLE \"EmployeeDegree\"';"
                   "  EXCEPTION "
                   "    WHEN OTHERS THEN "
                   "      IF SQLCODE != -942 THEN RAISE; END IF;"
                   "END;");

      db->execute (
        "CREATE TABLE \"Employer\" ("
        "\"name\" VARCHAR (255) PRIMARY KEY)");

      db->execute (
        "CREATE TABLE \"Employee\" ("
        "\"ssn\" NUMBER(10) PRIMARY KEY,"
        "\"first_name\" VARCHAR (255) NOT NULL,"
        "\"last_name\" VARCHAR (255) NOT NULL,"
        "\"employer\" VARCHAR (255) NOT NULL)");

      db->execute (
        "CREATE TABLE \"EmployeeDegree\" ("
        "\"ssn\" NUMBER(10) NOT NULL,"
        "\"degree\" VARCHAR (255) NOT NULL)");

      db->execute (
        "ALTER TABLE \"Employee\" "
        "ADD FOREIGN KEY (\"employer\") "
        "REFERENCES \"Employer\" "
        "INITIALLY DEFERRED");

      db->execute (
        "ALTER TABLE \"EmployeeDegree\" "
        "ADD FOREIGN KEY (\"ssn\") "
        "REFERENCES \"Employee\" "
        "INITIALLY DEFERRED");

      t.commit ();
    }
#else
#  error unknown database
#endif

    // Create a few persistent objects.
    //
    {
      shared_ptr<employer> st (new employer ("Simple Tech Ltd"));

      shared_ptr<employee> john (new employee (1, "John", "Doe", st));
      shared_ptr<employee> jane (new employee (2, "Jane", "Doe", st));

      john->degrees ().push_back ("BA");
      john->degrees ().push_back ("MSc");
      jane->degrees ().push_back ("Ph.D.");

      transaction t (db->begin ());

      db->persist (st);
      db->persist (john);
      db->persist (jane);

      t.commit ();
    }

    // Load employees with "Doe" as the last name and print what we've got.
    //
    {
      typedef odb::query<employee> query;
      typedef odb::result<employee> result;

      session s;
      transaction t (db->begin ());

      result r (db->query<employee> (query::name.last == "Doe"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        cout << i->name ().first () << " " << i->name ().last () << endl
             << "  employer: " << i->employer ()->name () << endl;

        for (degrees::iterator j (i->degrees ().begin ());
             j != i->degrees ().end ();
             ++j)
        {
          cout << "  degree: " << *j << endl;
        }

        cout << endl;
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
