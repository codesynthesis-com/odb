// file      : common/types/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test C++ type handling (anonymous types, aliasing).
//

#include <memory>
#include <cassert>
#include <iostream>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

template <typename T1, typename T2>
struct same_p
{
  static const bool result = false;
};

template <typename T>
struct same_p<T, T>
{
  static const bool result = true;
};

int
main ()
{
  assert ((same_p<odb::object_traits<object2>::id_type, int>::result));
}
