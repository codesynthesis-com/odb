// file      : qt/common/basic/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt basic type persistence (common part).
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    object o (1);
    o.uuid_ = QUuid::createUuid ();
    o.null_ = QUuid ();
    o.zero_ = QUuid ();

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> p (db->load<object> (o.id_));
      t.commit ();

      assert (*p == o);
    }

    {
      typedef odb::query<object> query;
      typedef odb::result<object> result;

      transaction t (db->begin ());
      result r (db->query<object> (query::uuid == o.uuid_));
      result::iterator i (r.begin ());
      assert (i != r.end () && i->id_ == o.id_);
      assert (++i == r.end ());
      t.commit ();
    }

  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
