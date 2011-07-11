// file      : common/callback/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test database operation callbacks.
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

const char* events[] =
{
  "pre_persist",
  "post_persist",
  "pre_load",
  "post_load",
  "pre_update",
  "post_update",
  "pre_erase",
  "post_erase"
};

void object::
db_callback (callback_event e, database&)
{
  cout << "  " << events[e] << " " << id_ << endl;
}

void object::
db_callback (callback_event e, database&) const
{
  cout << "  " << events[e] << " " << id_ << " const" << endl;
}

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Persist.
    //
    cout << "persist" << endl;
    {
      object o1 (1, 1);
      object const o2 (2, 2);
      transaction t (db->begin ());
      db->persist (o1);
      db->persist (&o2);
      t.commit ();
    }
    cout << "***" << endl;

    // Load.
    //
    cout << "load" << endl;
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      object o2;
      db->load<object> (2, o2);
      t.commit ();
    }
    cout << "***" << endl;

    // Query.
    //
    cout << "query" << endl;
    {
      typedef odb::query<object> query;
      typedef odb::result<object> result;

      transaction t (db->begin ());
      result r (db->query<object> ((query::id < 3) + "ORDER BY id"));

      for (result::iterator i (r.begin ()); i != r.end (); ++i)
      {
        if (i->id_ > 3) // Load.
          break;
      }

      t.commit ();
    }
    cout << "***" << endl;

    // Update.
    //
    cout << "update" << endl;
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      auto_ptr<object> o2 (db->load<object> (2));
      o1->data++;
      o2->data++;
      db->update (o1.get ());
      db->update (static_cast<object const&> (*o2));
      t.commit ();
    }
    cout << "***" << endl;

    // Erase.
    //
    cout << "erase" << endl;
    {
      transaction t (db->begin ());
      auto_ptr<object> o1 (db->load<object> (1));
      auto_ptr<object> o2 (db->load<object> (2));
      db->erase (static_cast<object const*> (o1.get ()));
      db->erase (*o2);
      t.commit ();
    }
    cout << "***" << endl;

    // Delayed (recursive) load.
    //
    cout << "delayed load" << endl;
    {
      {
        object o1 (1, 1);
        object o2 (2, 2);
        object o3 (3, 2);

        o1.pobj = &o2;
        o2.pobj = &o3;

        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object> o1 (db->load<object> (1));
        delete o1->pobj->pobj;
        delete o1->pobj;
        t.commit ();
      }
    }
    cout << "***" << endl;
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
