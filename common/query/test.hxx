// file      : common/query/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include <common/config.hxx>  // DATABASE_XXX

typedef odb::nullable<std::vector<char> > nullable_vector;

#pragma db object
struct person
{
  person (unsigned long id,
          const std::string& fn,
          const std::string& ln,
          unsigned short age,
          bool married,
          const nullable_vector& public_key = nullable_vector ())
      : id_ (id),
        first_name_ (fn),
        last_name_ (ln),
        age_ (age),
        married_ (married),
        public_key_ (public_key)
  {
  }

  person ()
  {
  }

  #pragma db id
  unsigned long id_;

  #pragma db column ("first")
  std::string first_name_;

  #pragma db column ("middle") null
  std::auto_ptr<std::string> middle_name_;

  #pragma db column ("last")
  std::string last_name_;

  unsigned short age_;
  bool married_;

#ifdef DATABASE_PGSQL
  #pragma db column ("key") type ("BYTEA") null
#else
  #pragma db column ("key") type ("BLOB") null
#endif
  nullable_vector public_key_;
};

inline std::ostream&
operator<< (std::ostream& os, const person& p)
{
  os << p.first_name_;

  if (p.middle_name_.get () != 0)
    os << ' '  << *p.middle_name_;

  os << ' ' << p.last_name_ << ' ' << p.age_ <<
    (p.married_ ? " married" : " single");

  if (p.public_key_ && p.public_key_->size () > 0)
  {
    os << ' ';

    for (std::size_t i (0), e (p.public_key_->size () - 1); i < e; ++i)
      os << (unsigned int)(*p.public_key_)[i] << '-';

    os << (unsigned int)p.public_key_->back ();
  }

  return os;
}

#endif // TEST_HXX
