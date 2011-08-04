// file      : common/wrapper/driver.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
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
    //
    unsigned long id;
    {
      object o;
      o.num.reset (new unsigned long (123));
      o.nstrs.push_back (nullable_string ());
      o.nstrs.push_back (nullable_string ("123"));
#ifdef HAVE_TR1_MEMORY
      o.tr1_strs.push_back (tr1_nullable_string ());
      o.tr1_strs.push_back (tr1_nullable_string (new string ("123")));
#endif

      transaction t (db->begin ());
      id = db->persist (o);
      t.commit ();
    }

    //
    //
    {
      transaction t (db->begin ());
      auto_ptr<object> o (db->load<object> (id));
      t.commit ();

      assert (*o->num == 123);
      assert (o->str.get () == 0);
      assert (o->nstr.null ());
      assert (o->nstrs[0].null ());
      assert (o->nstrs[1].get () == "123");
#ifdef HAVE_TR1_MEMORY
      assert (!o->tr1_str);
      assert (!o->tr1_strs[0]);
      assert (*o->tr1_strs[1] == "123");
#endif
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
