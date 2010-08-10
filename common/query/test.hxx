// file      : common/query/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <iostream>

#include <odb/core.hxx>

#pragma odb object
struct person
{
  person (unsigned long id,
          const std::string& fn,
          const std::string& ln,
          unsigned short age)
      : id_ (id), first_name_ (fn), last_name_ (ln), age_ (age)
  {
  }

  person ()
  {
  }

  #pragma odb id
  unsigned long id_;

  std::string first_name_;
  std::string last_name_;
  unsigned short age_;
};

inline std::ostream&
operator<< (std::ostream& os, const person& p)
{
  return os << p.first_name_ << ' ' << p.last_name_ << ' ' << p.age_;
}

#endif // TEST_HXX
