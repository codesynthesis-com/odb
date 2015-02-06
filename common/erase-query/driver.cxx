// file      : common/erase-query/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test query-based erase.
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

      if (db->id () != odb::id_oracle)
        assert (db->erase_query<object> (
                  "erase_query_object.id < 3") == 2);
      else
        assert (db->erase_query<object> (
                  "\"erase_query_object\".\"id\" < 3") == 2);

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

    // Test predicates involving object pointers (DELETE JOIN).
    //
    /*
    {
      object o11 (1);
      object o12 (2);
      object o13 (3);
      object2 o2;

      o11.o2 = &o2;
      o2.num = 123;

      o12.o1 = &o13;
      o13.num = 123;

      transaction t (db->begin ());
      db->persist (o2);
      db->persist (o13);
      db->persist (o12);
      db->persist (o11);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      assert (db->erase_query<object> (query::o1::num == 123) == 1);
      assert (db->erase_query<object> (query::o2::num == 123) == 1);
      db->erase_query<object> ();
      t.commit ();
    }
    */

    // For now we can only do column-based tests, like is_null().
    //
    {
      object o11 (1);
      object o12 (2);
      object o13 (3);
      object2 o2;

      o12.o2 = &o2;

      transaction t (db->begin ());
      db->persist (o2);
      db->persist (o13);
      db->persist (o12);
      db->persist (o11);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      assert (db->erase_query<object> (query::o2.is_null ()) == 2);
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

    {
      transaction t (db->begin ());

      if (db->id () != odb::id_oracle)
        assert (db->execute ("SELECT * FROM erase_query_object_v "
                             "WHERE object_id = 1") == 0);
      else
        assert (db->execute ("SELECT * FROM \"erase_query_object_v\" "
                             "WHERE \"object_id\" = 1") == 0);
      t.commit ();
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
