// file      : common/query/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <memory>
#include <iostream>

#include <odb/core.hxx>
#include <common/config.hxx>  // DATABASE_XXX

#pragma db object
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

  #pragma db id
  unsigned long id_;

  #pragma db column ("first")
  std::string first_name_;
#ifndef DATABASE_ORACLE
  #pragma db column ("middle") type ("TEXT") null
#else
  #pragma db column ("middle") type ("CLOB") null
#endif
  std::auto_ptr<std::string> middle_name_;

  #pragma db column ("last")
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
