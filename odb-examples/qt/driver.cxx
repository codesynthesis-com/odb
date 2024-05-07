// file      : qt/driver.cxx
// copyright : not copyrighted - public domain

#include <iostream>

#include <QtCore/QCoreApplication>

#include <odb/database.hxx>
#include <odb/session.hxx>
#include <odb/transaction.hxx>

#include "database.hxx" // createDatabase

#include "employee.hxx"
#include "employee-odb.hxx"

using namespace std;
using namespace odb::core;

ostream&
operator<< (ostream& os, const QString& s)
{
  return os << s.toStdString ();
}

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    auto_ptr<database> db (createDatabase (argc, argv));

    // Create a few persistent objects.
    //
    {
      // Simple Tech Ltd.
      //
      {
        QSharedPointer<Employer> er (new Employer ("Simple Tech Ltd"));

        QSharedPointer<Employee> john (
          new Employee ("John",
                        "Doe",
                        QDate (1975, 1, 1),
                        QByteArray ("\0xF1\0x00\0x34\0x45\0x00\0xDE", 6),
                        er));

        QSharedPointer<Employee> jane (
          new Employee ("Jane",
                        "Doe",
                        QDate (1976, 2, 2),
                        QByteArray ("\0xD7\0x00\0x14", 3),
                        er));

        john->emails ().insert ("john_d@example.com");
        john->emails ().insert ("john.doe@simple.com");
        jane->emails ().insert ("jane_d@example.com");
        jane->emails ().insert ("jane.doe@simple.com");

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
        QSharedPointer<Employer> er (new Employer ("Complex Systems Inc"));

        QSharedPointer<Employee> john (
          new Employee ("John",
                        "Smith",
                        QDate (1977, 3, 3),
                        QByteArray ("\0x23\0xFD\0x8F\0x00", 4),
                        er));

        QSharedPointer<Employee> jane (
          new Employee ("Jane",
                        "Smith",
                        QDate (1978, 4, 4),
                        QByteArray ("0x00\0x32\0x00\0x01\0x00", 5),
                        er));

        john->emails ().insert ("john_s@example.com");
        john->emails ().insert ("john.smith@complex.com");
        jane->emails ().insert ("jane_s@example.com");
        jane->emails ().insert ("jane.smith@complex.com");

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

      QSharedPointer<Employer> stl (db->load<Employer> ("Simple Tech Ltd"));

      Employees& es (stl->employees ());

      for (Employees::iterator i (es.begin ()); i != es.end (); ++i)
      {
        QLazyWeakPointer<Employee>& lwp (*i);

        // Load and lock the employee and his employer.
        //
        QSharedPointer<Employee> p (lwp.load ());
        QSharedPointer<Employer> pe (p->employer ().load ());

        cout << p->first () << " " << p->last ()  << endl
             << "  born: " << p->born ().toString () << endl;

        for (Emails::const_iterator j (p->emails ().begin ()),
               e (p->emails ().end ()); j != e; ++j)
        {
          cout << "  email: " << *j << endl;
        }

        cout  << "  public key length: " << p->publicKey ().size () << endl
              << "  employer: " << pe->name () << endl
              << "  id: " << p->id ().toString () << endl
              << endl;
      }

      t.commit ();
    }

    // Search for Complex Systems Inc employees that were born before
    // April 1978.
    //
    {
      typedef odb::query<Employee> query;
      typedef odb::result<Employee> result;

      session s;
      transaction t (db->begin ());

      result r (db->query<Employee> (
                  query::employer->name == "Complex Systems Inc" &&
                  query::born < QDate (1978, 4, 1)));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
        cout << i->first () << " " << i->last ()
             << " " << i->born ().toString () << endl;

      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
