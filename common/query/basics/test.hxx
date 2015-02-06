// file      : common/query/basics/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_PGSQL)
#    define BLOB_TYPE "BYTEA"
#  elif defined(ODB_DATABASE_MSSQL)
#    define BLOB_TYPE "VARBINARY(max)"
#  else
#    define BLOB_TYPE "BLOB"
#  endif
#endif

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

  #pragma db column ("key") type(BLOB_TYPE) null
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
