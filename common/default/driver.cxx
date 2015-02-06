// file      : common/default/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test default values.
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

    // Insert an object using an ad-hoc SQL statement. This way
    // we get all the default values.
    //
    {
      transaction t (db->begin ());

      if (db->id () != odb::id_oracle)
        db->execute ("INSERT INTO default_object (obj_id) VALUES (1)");
      else
        db->execute ("INSERT INTO \"default_object\" (\"obj_id\") VALUES (1)");

      t.commit ();
    }

    // Now load the object and check all the values.
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (1));
      t.commit ();

      assert (o->b);
      assert (o->pi == 1234);
      assert (o->ni == -1234);
      assert (o->zi == 0);
      assert (o->pf == 1.234);
      assert (o->nf == -1.234);
      assert (o->zf == 0.0);
      assert (o->sf == 1.123e+10);
      assert (o->str == "Someone's string");
      assert (o->e == green);
    }

    // Check the NULL default value using a query.
    //
    {
      typedef odb::query<object> query;
      typedef odb::result<object> result;

      transaction t (db->begin ());
      result r (db->query<object> (query::null.is_null ()));
      assert (!r.empty ());
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
