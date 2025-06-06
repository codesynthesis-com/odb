// file      : qt/pgsql/basic/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt basic type persistence. PostgreSQL version.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <QtCore/QCoreApplication>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    unique_ptr<database> db (create_database (argc, argv));

    object o;
    o.str = "Constantin Michael";
    o.blob = QByteArray ("\0x13\0xDE\0x00\0x00\0x00\0x54\0xF2\0x6A", 8);

    // Persist.
    //
    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    // Load.
    //
    {
      transaction t (db->begin ());
      unique_ptr<object> p (db->load<object> (o.str));
      t.commit ();

      assert (*p == o);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
