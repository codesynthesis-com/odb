// file      : pgsql/index/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  int i;
  #pragma db index type("UNIQUE CONCURRENTLY") method("BTREE") member(i)
};

#endif // TEST_HXX
