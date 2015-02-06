// file      : common/relationship/on-delete/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id;
};

#pragma db object
struct cascade
{
  #pragma db id auto
  unsigned long id;

  #pragma db on_delete(cascade)
  object* p;
};

#pragma db object
struct cascade_cont
{
  #pragma db id auto
  unsigned long id;

  #pragma db on_delete(cascade)
  std::vector<object*> p;
};

#pragma db object
struct set_null
{
  #pragma db id auto
  unsigned long id;

  #pragma db on_delete(set_null)
  object* p;
};

#pragma db object
struct set_null_cont
{
  #pragma db id auto
  unsigned long id;

  #pragma db on_delete(set_null)
  std::vector<object*> p;
};

#endif // TEST_HXX
