// file      : qt/common/smart-ptr/driver.cxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test Qt smart pointers.
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

    QSharedPointer<cont> c1 (new cont (1));

    // Test comparison operators.
    //
    {
      assert (QLazySharedPointer<cont> () == QLazySharedPointer<cont> ());
      assert (QLazySharedPointer<cont> () != QLazySharedPointer<cont> (c1));
      assert (QLazySharedPointer<cont> (c1) == QLazySharedPointer<cont> (c1));

      QLazySharedPointer<cont> lc1 (*db, 1);
      assert (lc1 != QLazySharedPointer<cont> ());
      assert (lc1 == QLazySharedPointer<cont> (*db, c1));

      QSharedPointer<cont> c2 (new cont (2));
      assert (lc1 != QLazySharedPointer<cont> (*db, c2));
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
