// file      : prepared/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>
#include <cstddef> // std::size_t

#include <odb/core.hxx>

#pragma db object
class person
{
public:
  person (const std::string& first,
          const std::string& last,
          unsigned short age)
      : first_ (first), last_ (last), age_ (age)
  {
  }

  const std::string&
  first () const
  {
    return first_;
  }

  const std::string&
  last () const
  {
    return last_;
  }

  unsigned short
  age () const
  {
    return age_;
  }

private:
  friend class odb::access;

  person () {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;
  unsigned short age_;
};

#pragma db view object(person)
struct person_count
{
  #pragma db column("count(" + person::id_ + ")")
  std::size_t count;
};

#endif // PERSON_HXX
