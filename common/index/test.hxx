// file      : common/index/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>

// Test basic functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object
  struct object
  {
    #pragma db id auto
    unsigned long id_;

    #pragma db index
    int i1;

    #pragma db unique
    int i2;

    #pragma db unique index
    int i3;

    int i4;
    #pragma db index unique member(i4)

    int i5;
    #pragma db index type("UNIQUE") member(i5)

    int i6;
    #pragma db index("object_i6_index") member(i6)

    int i7;
    int i8;
    int i9;

    int i10;
    #pragma db index member(i10, "ASC")
  };

  #pragma db index(object) member(i7)
  #pragma db index(object::"object_i8_index") member(i8)
}

#pragma db index(test1::object::"object_i9_index") member(i9)

// Test composite indexes.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db value
  struct nested
  {
    int x;
    int y;
  };

  #pragma db value
  struct comp
  {
    int x;
    int y;
    nested n;
  };

  #pragma db object
  struct object
  {
    #pragma db id auto
    unsigned long id_;

    int i1a;
    int i1b;
    #pragma db index("object_i1_i") member(i1a) member(i1b)

    int i2a;
    int i2b;
    #pragma db index("object_i2_i") members(i2a, i2b)

    #pragma db index
    comp c1;

    #pragma db index column("")
    comp c2;

    comp c3;
    #pragma db index member(c3.x)
    #pragma db index member(c3.y)

    comp c4;
    #pragma db index("object_c4_i") members(c4.x, c4.y, c4.n.x)

    comp c5;
    int i5;
    #pragma db index("object_ci5_i") member(c5) member(i5)
  };
}

// Test container indexes.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db value
  struct id
  {
    int x;
    int y;
  };

  #pragma db value
  struct comp
  {
    int x;
    std::vector<int> v;
  };

  #pragma db object
  struct object
  {
    #pragma db id
    id id_;

    std::vector<int> v;
    #pragma db index unique member(v.id)
    #pragma db index("object_v_index_index") member(v.index)

    comp c;
    #pragma db index("object_c_v_id_index") member(c.v.id)
    #pragma db index unique member(c.v.index)
  };
}

#endif // TEST_HXX
