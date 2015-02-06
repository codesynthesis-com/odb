// file      : qt/oracle/basic/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt basic type persistence. Oracle version.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>
#include <string>

#include <QtCore/QCoreApplication>

#include <odb/oracle/database.hxx>
#include <odb/oracle/transaction.hxx>

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

    string short_str (13, 's');
    string medium_str (150, 'm');
    string long_str (20000, 'v');
    // Oracle UTF-8 support is limited to 3-byte sequences.
    //
    string unicode_str ("a \xD5\x95 \xEA\xAA\xAA \xE2\x82\xAC bcdef");

    object o;

    o.char_= QString::fromStdString (short_str);
    o.varchar2 = QString::fromStdString (medium_str);
    o.clob = QString::fromStdString (long_str);

    // fromStdString() assumes ASCII in Qt4 and UTF-8 in Qt5.
    //
    o.nchar= QString::fromUtf8 (unicode_str.c_str (),
                                static_cast<int> (unicode_str.size ()));
    o.nvarchar2 = QString::fromUtf8 (unicode_str.c_str (),
                                     static_cast<int> (unicode_str.size ()));
    o.nclob = QString::fromStdString (long_str);

    o.raw = QByteArray ("\0x13\0xDE\0x00\0x00\0x00\0x54\0xF2\0x6A", 8);
    o.blob = QByteArray (long_str.c_str (),
                         static_cast<int> (long_str.size ()));

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
      auto_ptr<object> p (db->load<object> (o.varchar2));
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
