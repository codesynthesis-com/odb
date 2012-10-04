// file      : common/prepared/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object
struct person
{
  person () {}
  person (const std::string& name, unsigned short age)
      : name_ (name), age_ (age) {}

  #pragma db id auto
  unsigned long id_;

  std::string name_;
  unsigned short age_;
};

#endif // TEST_HXX
