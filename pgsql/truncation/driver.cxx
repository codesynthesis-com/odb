// file      : pgsql/truncation/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test insufficient buffer/truncation handling.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  // The default pre-allocated buffer is 512 bytes long.
  //
  string long_str (640, 'c'); // This will get the buffer to 1024
  string longer_str (1025, 'b');

  try
  {
    // Test basic operations.
    //
    {
      auto_ptr<database> db (create_specific_database<database> (argc, argv));

      // Run persist/load so that the initial bindings are established
      // (version == 0).
      //
      {
        object1 o (1);
        o.str_ = "test string";

        transaction t (db->begin ());
        db->persist (o);
        db->load (1, o);
        t.commit ();
      }

      {
        object2 o (2);
        o.str_ = "test string";

        transaction t (db->begin ());
        db->persist (o);
        db->load (2, o);
        t.commit ();
      }

      // Store/load the long string which should trigger buffer growth.
      //
      {
        object1 o (3);
        o.str_ = long_str;

        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object2> o (db->load<object2> (3));
        assert (o->str_ == long_str);
        t.commit ();
      }

      // Store/load longer string.
      //
      {
        object1 o (3);
        o.str_ = longer_str;

        transaction t (db->begin ());
        db->update (o);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<object2> o (db->load<object2> (3));
        assert (o->str_ == longer_str);
        t.commit ();
      }
    }

    // Test query.
    //
    {
      typedef pgsql::query<object1> query;
      typedef odb::result<object1> result;

      auto_ptr<database> db (create_specific_database<database> (argc, argv));

      // Run persist/query so that the initial bindings are established
      // (version == 0).
      //
      {
        object1 o (20);
        o.str_ = "test string";

        transaction t (db->begin ());
        db->persist (o);
        o.id_++;
        db->persist (o);
        o.id_++;
        db->persist (o);

        result r (db->query<object1> (query::id == 20));
        assert (r.begin ()->id_ == 20);
        t.commit ();
      }

      // Test buffer growth with cached result.
      //
      {
        object1 o;

        transaction t (db->begin ());

        result r (db->query<object1> (query::id >= 20));
        result::iterator i (r.begin ());

        o.id_ = i->id_;
        o.str_ = long_str;

        // This forces buffer growth in the middle of result iteration.
        //
        db->update (o);

        ++i;
        assert (i->str_ == "test string");

        o.id_ = i->id_;
        o.str_ = longer_str;
        db->update (o);

        ++i;
        assert (i->str_ == "test string");

        t.commit ();
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
