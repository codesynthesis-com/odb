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

typedef std::vector<char> buffer;
typedef odb::nullable<buffer> nullable_buffer;

#pragma db object
struct person
{
  person (unsigned long id,
          const std::string& fn,
          const std::string& ln,
          unsigned short age,
          bool married,
          const nullable_buffer& public_key = nullable_buffer ())
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

#ifdef ODB_DATABASE_PGSQL
  #pragma db column ("key") type ("BYTEA") null
#else
  #pragma db column ("key") type ("BLOB") null
#endif
  nullable_buffer public_key_;
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
