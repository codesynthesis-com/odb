// file      : common/definition/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test overriding composite value definition point. This is primarily
// useful to make composite values out of third-party types.
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

    object o;
    o.time.tv_sec = 1;
    o.time.tv_usec = 1000;

    {
      transaction t (db->begin ());
      db->persist (o);
      t.commit ();
    }

    {
      transaction t (db->begin ());
      auto_ptr<object> p (db->load<object> (o.id));
      t.commit ();

      assert (p->time.tv_sec == o.time.tv_sec &&
              p->time.tv_usec == o.time.tv_usec);
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
