// file      : common/enum/test.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>

enum color {red, green, blue};

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  color color_;
  enum taste {bitter, sweet, sour};
  taste taste_;
};

inline bool
operator == (const object& x, const object& y)
{
  return
    x.id_ == y.id_ &&
    x.color_ == y.color_ &&
    x.taste_ == y.taste_;
}

#endif // TEST_HXX
