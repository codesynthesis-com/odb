// file      : common/changelog/add-table.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ADD_TABLE_HXX
#define ADD_TABLE_HXX

#include <vector>

#pragma db model version(BVER, CVER, open)

struct object1;

#if CVER > 1
#pragma db object
struct object
{
  #pragma db id auto
  int id;
  int num;

  std::vector<int> nums;
  object1* o1;
};
#endif

#pragma db object
struct object1
{
  #pragma db id
  int id;
  int num;
};

#endif // ADD_TABLE_HXX
