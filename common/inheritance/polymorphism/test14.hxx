// file      : common/inheritance/polymorphism/test14.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST14_HXX
#define TEST14_HXX

#include <vector>

#include <odb/core.hxx>

// Test inverse pointer in polymorphic base.
//
#pragma db namespace table("t14_")
namespace test14
{
  struct object1;
  struct object2;
  struct object3;
  struct object4;

  #pragma db object polymorphic session
  struct base
  {
    virtual ~base ();

    #pragma db id auto
    unsigned long id;

    object1* o1;
    object2* o2;
    std::vector<object3*> o3;
    std::vector<object4*> o4;
  };

  #pragma db object
  struct derived: base
  {
    unsigned long num;
  };

  // one-to-one(i)
  //
  #pragma db object session
  struct object1
  {
    #pragma db id auto
    unsigned long id;

    #pragma db inverse(o1)
    derived* d;
  };

  // one-to-many(i)
  //
  #pragma db object session
  struct object2
  {
    #pragma db id auto
    unsigned long id;

    #pragma db inverse(o2)
    std::vector<derived*> d;
  };

  // many-to-many(i)
  //
  #pragma db object session
  struct object3
  {
    #pragma db id auto
    unsigned long id;

    #pragma db inverse(o3)
    std::vector<derived*> d;
  };

  // many-to-one(i)
  //
  #pragma db object session
  struct object4
  {
    #pragma db id auto
    unsigned long id;

    #pragma db inverse(o4)
    derived* d;
  };

  inline base::
  ~base ()
  {
    delete o1;
    delete o2;
    delete o3[0];
    delete o4[0];
  }
}

#endif // TEST14_HXX
