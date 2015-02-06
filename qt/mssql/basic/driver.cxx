// file      : qt/mssql/basic/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt basic type persistence. SQL Server version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <QtCore/QCoreApplication>

#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  QCoreApplication app (argc, argv);

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    object o;
    o.id_ = "object 1";
    o.sstr_ = QString (512, 's');
    o.lstr_ = QString (65000, 'l'); // Test temp buffer boundary.
    o.snstr_ = QString (512, QChar (0x1234));
    o.lnstr_ = QString (65536, QChar (0x2345));
    o.sbuf_ = QByteArray (512, 0x01);
    o.lbuf_ = QByteArray (65536, 0x02);

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
      auto_ptr<object> p (db->load<object> (o.id_));
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
