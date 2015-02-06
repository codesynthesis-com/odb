// file      : common/container/change-tracking/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <string>
#include <memory>
#include <vector>

#ifdef HAVE_CXX11
#  include <utility> // std::move
#endif

#include <odb/core.hxx>
#include <odb/vector.hxx>

#pragma db object pointer(std::auto_ptr)
struct object
{
  object () {}
  object (const std::string& id): id_ (id) {}

#ifdef HAVE_CXX11
  object (const object& x): id_ (x.id_), i (x.i), s (x.s) {}
  object (object&& x): id_ (std::move (x.id_)), i (x.i), s (std::move (x.s)) {}
#endif

  #pragma db id
  std::string id_;

  unsigned int i;

  odb::vector<std::string> s;

  inline bool
  operator== (const object& o) {return id_ == o.id_ && i == o.i && s == o.s;}
};

// Test mixing "smart" and "dumb" container (specifically, erase(obj)).
//
#pragma db object
struct mix_object
{
  mix_object () {}
  mix_object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  odb::vector<int> ov;
  std::vector<int> sv;
};

// Test using change tracking container as inverse member.
//
struct inv_object2;

#pragma db object session
struct inv_object1
{
  #pragma db id auto
  unsigned long id_;

  inv_object2* o2;
};

#pragma db object session
struct inv_object2
{
  #pragma db id auto
  unsigned long id_;

  #pragma db inverse(o2)
  odb::vector<inv_object1*> o1;
};

// Test read-only values (we still need to include them in the UPDATE
// statement).
//
#pragma db value
struct ro_value
{
  ro_value (int i_ = 0, int j_ = 0): i (i_), j (j_) {}

  #pragma db readonly
  int i;

  #pragma db readonly
  int j;
};

inline bool
operator== (const ro_value& x, const ro_value& y)
{
  return x.i == y.i && x.j == y.j;
}

#pragma db object
struct ro_object
{
  ro_object () {}
  ro_object (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  odb::vector<ro_value> v;
};

#endif // TEST_HXX
