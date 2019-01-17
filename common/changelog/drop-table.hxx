// file      : common/changelog/drop-table.hxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef DROP_TABLE_HXX
#define DROP_TABLE_HXX

#include <vector>

#pragma db model version(BVER, CVER, open)

struct object1;

#if CVER == 1
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

#endif // DROP_TABLE_HXX
