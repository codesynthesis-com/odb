// file      : mysql/truncation/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test insufficient buffer/truncation handling.
//

#include <memory>   // std::unique_ptr
#include <iostream>

#include <odb/mysql/database.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace mysql = odb::mysql;
using namespace mysql;

int
main (int argc, char* argv[])
{
  // The default pre-allocated buffer is 256 bytes long.
  //
  string long_str (300, 'c'); // This will get the buffer to 512
  string longer_str (1025, 'b');

  try
  {
    // Test basic operations.
    //
    {
      unique_ptr<database> db (create_specific_database<database> (argc, argv));

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
        unique_ptr<object2> o (db->load<object2> (3));
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
        unique_ptr<object2> o (db->load<object2> (3));
        assert (o->str_ == longer_str);
        t.commit ();
      }
    }

    // Test query.
    //
    {
      typedef mysql::query<object1> query;
      typedef odb::result<object1> result;

      unique_ptr<database> db (create_specific_database<database> (argc, argv));

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

    // Test containers.
    //
    {
      unique_ptr<database> db (create_specific_database<database> (argc, argv));

      // Use different connections to persist and load the object.
      //
      connection_ptr c1 (db->connection ());
      connection_ptr c2 (db->connection ());

      container o (1);
      o.vec_.push_back (string (513, 'x'));

      {
        transaction t (c1->begin ());
        db->persist (o);
        t.commit ();
      }

      {
        transaction t (c2->begin ());
        unique_ptr<container> p (db->load<container> (1));
        t.commit ();

        assert (p->vec_ == o.vec_);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
