// file      : common/erase-query/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query-based erase.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/config.hxx> // @@ DATABASE_MYSQL
#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

void
persist (database& db)
{
  object o1 (1);
  object o2 (2);
  object o3 (3);
  object o4 (4);

  transaction t (db.begin ());
  db.persist (o1);
  db.persist (o2);
  db.persist (o3);
  db.persist (o4);
  t.commit ();
}

int
main (int argc, char* argv[])
{
  try
  {
    typedef odb::query<object> query;

    auto_ptr<database> db (create_database (argc, argv));

    // erase_query()
    //
    persist (*db);

    {
      transaction t (db->begin ());
      assert (db->erase_query<object> () == 4);
      t.commit ();
    }

    // erase_query(const char*)
    //
    persist (*db);

    {
      transaction t (db->begin ());
      assert (db->erase_query<object> ("id < 3") == 2);
      db->erase_query<object> ();
      t.commit ();
    }

    // erase_query(query)
    //
    persist (*db);

    {
      transaction t (db->begin ());
      assert (db->erase_query<object> (query::id == 2 || query::id == 4) == 2);
      db->erase_query<object> ();
      t.commit ();
    }

    // Make sure container data is deleted.
    //
    {
      object o (1);
      o.v.push_back (1);
      o.v.push_back (2);
      o.v.push_back (3);

      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      assert (db->erase_query<object> () == 1);
      t.commit ();
    }

    // @@ Temporarily disabled until we have out-of-line constraints
    // in MySQL (coming with evolution support).
    //
#ifndef DATABASE_MYSQL
    {
      transaction t (db->begin ());
      assert (db->execute ("SELECT * FROM object_v WHERE object_id = 1") == 0);
      t.commit ();
    }
#endif
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
