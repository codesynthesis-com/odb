// file      : common/changelog/model.hxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_HXX
#define MODEL_HXX

#include <vector>

#pragma db model version(1, 1, open)

#pragma db value
struct value
{
  int x;
  int y;
};

struct object1;

#pragma db object
struct object
{
  #pragma db id auto
  int id;

  #pragma db null default(0) options("DUMMY=1")
  int num;

  #pragma db index unique member(num, "DESC") method("BTREE") options("DUMMY=1")

  #pragma db index
  value v; // Multi-column.

  std::vector<int> nums;
  object1* o1;
};

#pragma db object
struct object1
{
  #pragma db id
  value id; // Multi-column.
  int num;
};

#endif // MODEL_HXX
