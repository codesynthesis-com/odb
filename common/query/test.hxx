// file      : common/query/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <memory>
#include <iostream>

#include <odb/core.hxx>

#pragma odb object
struct person
{
  person (unsigned long id,
          const std::string& fn,
          const std::string& ln,
          unsigned short age,
          bool married)
      : id_ (id),
        first_name_ (fn),
        last_name_ (ln),
        age_ (age),
        married_ (married)
  {
  }

  person ()
  {
  }

  #pragma odb id
  unsigned long id_;

  #pragma odb column ("first")
  std::string first_name_;

  typedef std::auto_ptr<std::string> string_ptr; // @@ tmp
  #pragma odb column ("middle") type ("TEXT")
  string_ptr middle_name_;

  #pragma odb column ("last")
  std::string last_name_;

  unsigned short age_;
  bool married_;
};

inline std::ostream&
operator<< (std::ostream& os, const person& p)
{
  os << p.first_name_;

  if (p.middle_name_.get () != 0)
    os << ' '  << *p.middle_name_;

  os << ' ' << p.last_name_ << ' ' << p.age_ <<
    (p.married_ ? " married" : " single");

  return os;
}

#endif // TEST_HXX
