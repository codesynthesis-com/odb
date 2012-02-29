// file      : common/wrapper/driver.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test wrapper machinery.
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

    //
    // Simple values.
    //
    unsigned long id;
    {
      object o;
      o.num.reset (new int (123));
      o.nstrs.push_back (nullable_string ());
      o.nstrs.push_back (nullable_string ("123"));
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
      o.sstrs.push_back (str_sptr ());
      o.sstrs.push_back (str_sptr (new string ("123")));
#endif

      transaction t (db->begin ());
      id = db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (id));
      t.commit ();

      assert (*o->num == 123);
      assert (o->str.get () == 0);
      assert (o->nstr.null ());
      assert (o->nstrs[0].null ());
      assert (o->nstrs[1].get () == "123");
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
      assert (!o->sstr);
      assert (!o->sstrs[0]);
      assert (*o->sstrs[1] == "123");
#endif
    }

    //
    // Composite values.
    //
    {
      comp_object co;

      co.c1.reset (new comp1 ("123", 123));
      co.vc1.push_back (comp1 ("1", 1));
      co.vc1.push_back (comp1 ("2", 2));
      co.vc1.push_back (comp1 ("3", 3));

      co.c2.reset (new comp2 ("123", 123));
      co.c2->strs.push_back ("1");
      co.c2->strs.push_back ("2");
      co.c2->strs.push_back ("3");

      {
        transaction t (db->begin ());
        id = db->persist (co);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<comp_object> o (db->load<comp_object> (id));
        t.commit ();

        assert (*o->c1 == *co.c1);
        assert (o->vc1 == co.vc1);
        assert (*o->c2 == *co.c2);
      }
    }

    //
    // Containers.
    //
    {
      cont_object co;

      co.nums.reset (new vector<int>);
      co.nums->push_back (1);
      co.nums->push_back (2);
      co.nums->push_back (3);

      co.c.num = 123;
      co.c.strs.reset (new vector<string>);
      co.c.strs->push_back ("1");
      co.c.strs->push_back ("2");
      co.c.strs->push_back ("3");

      {
        transaction t (db->begin ());
        id = db->persist (co);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<cont_object> o (db->load<cont_object> (id));
        t.commit ();

        assert (*o->nums == *co.nums);
        assert (o->c == co.c);
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
