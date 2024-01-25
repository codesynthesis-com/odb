// file      : common/schema/embedded/order/test2.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST2_HXX
#define TEST2_HXX

#include <odb/core.hxx>

#include "test1.hxx"

#pragma db object
struct derived: base
{
  int num;
};

#endif // TEST2_HXX
