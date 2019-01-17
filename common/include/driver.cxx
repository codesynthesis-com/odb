// file      : common/include/driver.cxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test inclusion of -odb files (compilation test).
//
// The setup of this test is as follows: the ODB compiler has two
// additional include directories in its search path: .. and ../..
// while the C++ compiler has only ../.. . This way, if a ..-based
// path is used in the generated code, the C++ compilation will
// fail.
//

#include <memory>
#include <cassert>
#include <iostream>

#include <odb/exceptions.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test1.hxx"
#include "test1-odb.hxx"

#include "test2.hxx"
#include "test2-odb.hxx"

#include "test3.hxx"
#include "test3-odb.hxx"

#include "test4.hxx"
#include "test4-odb.hxx"

using namespace std;
using namespace odb::core;

int
main ()
{
}
