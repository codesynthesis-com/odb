// file      : boost/common/multi-index/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Boost multi-index container persistence.
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
  {
    using namespace odb;

    assert (odb::access::container_traits<int_lst>::kind == ck_ordered);
    assert (odb::access::container_traits<int_vec>::kind == ck_ordered);
    assert (odb::access::container_traits<int_set>::kind == ck_set);

    assert (odb::access::container_traits<int_lst_set>::kind == ck_ordered);
    assert (odb::access::container_traits<comp_set_vec>::kind == ck_ordered);
    assert (odb::access::container_traits<comp_set_set>::kind == ck_set);
  }

  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    for (unsigned short i (0); i < 2; ++i)
    {
      object empty ("empty"), med ("medium"), full ("full");

      //
      // empty
      //

      //
      // med
      //
      med.il.push_back (234);
      med.il.push_back (123);

      med.iv.push_back (234);
      med.iv.push_back (123);

      med.is.insert (234);
      med.is.insert (123);

      med.ils.push_back (234);
      med.ils.push_back (123);

      med.csv.insert (comp (234, "bcd"));
      med.csv.insert (comp (123, "abc"));

      med.css.insert (comp (234, "bcd"));
      med.css.insert (comp (123, "abc"));

      //
      // full
      //
      full.il.push_back (2345);
      full.il.push_back (1234);
      full.il.push_back (3456);

      full.iv.push_back (2345);
      full.iv.push_back (1234);
      full.iv.push_back (3456);

      full.is.insert (2345);
      full.is.insert (1234);
      full.is.insert (3456);

      full.ils.push_back (2345);
      full.ils.push_back (1234);
      full.ils.push_back (3456);

      full.csv.insert (comp (234, "bcde"));
      full.csv.insert (comp (123, "abcd"));
      full.csv.insert (comp (234, "cdef"));

      full.css.insert (comp (234, "bcde"));
      full.css.insert (comp (123, "abcd"));
      full.css.insert (comp (234, "cdef"));

      // persist
      //
      {
        transaction t (db->begin ());
        db->persist (empty);
        db->persist (med);
        db->persist (full);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> e (db->load<object> ("empty"));
        auto_ptr<object> m (db->load<object> ("medium"));
        auto_ptr<object> f (db->load<object> ("full"));
        t.commit ();

        assert (empty == *e);
        assert (med == *m);
        assert (full == *f);
      }

      // empty
      //
      empty.il.push_back (12);
      empty.iv.push_back (12);
      empty.is.insert (12);
      empty.ils.push_back (12);
      empty.csv.insert (comp (12, "ab"));
      empty.css.insert (comp (12, "ab"));

      // med
      //
      med.il.clear ();
      med.iv.clear ();
      med.is.clear ();
      med.ils.clear ();
      med.csv.clear ();
      med.css.clear ();

      // full
      //
      full.il.push_back (4567);
      full.iv.push_back (4567);
      full.is.insert (4567);
      full.ils.push_back (4567);
      full.csv.insert (comp (4567, "defg"));
      full.css.insert (comp (4567, "defg"));

      // update
      //
      {
        transaction t (db->begin ());
        db->update (empty);
        db->update (med);
        db->update (full);
        t.commit ();
      }

      // load & check
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> e (db->load<object> ("empty"));
        auto_ptr<object> m (db->load<object> ("medium"));
        auto_ptr<object> f (db->load<object> ("full"));
        t.commit ();

        assert (empty == *e);
        assert (med == *m);
        assert (full == *f);
      }

      // erase
      //
      if (i == 0)
      {
        transaction t (db->begin ());
        db->erase<object> ("empty");
        db->erase<object> ("medium");
        db->erase<object> ("full");
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
