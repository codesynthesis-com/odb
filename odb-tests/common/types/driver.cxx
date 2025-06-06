// file      : common/types/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test C++ type handling (anonymous types, aliasing).
//

#include <memory>
#include <iostream>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

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
