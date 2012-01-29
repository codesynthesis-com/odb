// file      : common/composite/test.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <utility> // std::pair

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
operator== (const person& x, const person& y)
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

// Test composite class template instantiation.
//
template <typename I, typename S>
struct comp
{
  I num;
  S str;
  std::vector<std::pair<I, S> > vec;
};

template <typename I, typename S>
inline bool
operator== (const comp<I, S>& x, const comp<I, S>& y)
{
  return x.num == y.num && x.str == y.str && x.vec == y.vec;
}

typedef std::pair<int, std::string> int_str_pair;
#pragma db value(int_str_pair)

// Make sure we use the name that was specified in the pragma.
//
#ifdef ODB_COMPILER
typedef comp<int, std::string> int_str_comp1;
#endif

typedef comp<int, std::string> int_str_comp;
#pragma db value(int_str_comp)

#pragma db object
struct object
{
  object (unsigned long id)
      : id_ (id)
  {
  }

  object ()
  {
  }

  #pragma db id
  unsigned long id_;

  comp<int, std::string> comp_;
  std::pair<int, std::string> pair_;
  std::vector<int_str_pair> vec_;
};

inline bool
operator== (const object& x, const object& y)
{
  return x.id_ == y.id_ &&
    x.comp_ == y.comp_ &&
    x.pair_ == y.pair_ &&
    x.vec_ == y.vec_;
}

#endif // TEST_HXX
