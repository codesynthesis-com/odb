// file      : common/optimistic/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

#pragma db object optimistic
struct object
{
  object (): ver (123) {}
  object (unsigned long id): id_ (id), ver (123) {}

  #pragma db id
  unsigned long id_;

  #pragma db version
  unsigned long ver;

  unsigned int num;
  std::string str;
};

#pragma db view object(object)
struct object_version
{
  unsigned long ver;
};

// Optimistic class with a container.
//
#pragma db object optimistic
struct container
{
  container (): ver (123) {}
  container (const std::string& id): id_ (id), ver (123) {}

  #pragma db id
  std::string id_;

  #pragma db version
  unsigned long ver;

  std::vector<unsigned int> nums;
};

// Optimistic class inheritance.
//
#pragma db object abstract optimistic
struct base
{
  base (): ver (123) {}

  #pragma db id auto
  unsigned long id_;

  #pragma db version
  const unsigned long ver;

  std::string str;

  #pragma db readonly
  std::string ro;
};

#pragma db object
struct derived: base
{
  unsigned int num;
};

#endif // TEST_HXX
