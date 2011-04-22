// file      : common/composite/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db value
struct name
{
  std::string first;
  std::string last;
};

#pragma db value
struct name_title
{
  std::string title;
};

#pragma db value
struct name_title_ex: name_title
{
  // Test value types without data members.
};

#pragma db value
struct name_flags
{
  bool nick;
  bool alias;
};

#pragma db value
struct name_ex: name, name_title_ex
{
  name alias;
  std::string nick;

  #pragma db column("show_")
  name_flags flags;
};

#pragma db object
struct person
{
  person (unsigned long id)
      : id_ (id)
  {
  }

  person ()
  {
  }

  #pragma db id
  unsigned long id_;

  #pragma db column("")
  name_ex name_;

  unsigned short age_;
};

inline bool
operator== (person const& x, person const& y)
{
  return x.id_ == y.id_ &&
    x.name_.first ==  y.name_.first&&
    x.name_.last == y.name_.last &&
    x.name_.title == y.name_.title &&
    x.name_.alias.first == y.name_.alias.first &&
    x.name_.alias.last == y.name_.alias.last &&
    x.name_.nick == y.name_.nick &&
    x.name_.flags.nick == y.name_.flags.nick &&
    x.name_.flags.alias == y.name_.flags.alias &&
    x.age_ == y.age_;
}

#endif // TEST_HXX
