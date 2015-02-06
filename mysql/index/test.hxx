// file      : mysql/template/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <odb/core.hxx>

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  std::string s;
  #pragma db index method("BTREE") member(s, "(200) DESC")
};

#endif // TEST_HXX
