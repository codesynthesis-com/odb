// file      : optimistic/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object optimistic
class person
{
public:
  person () {}
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

  void
  age (unsigned short age)
  {
    age_ = age;
  }

  unsigned long
  version () const
  {
    return version_;
  }

private:
  friend class odb::access;

  #pragma db id auto
  unsigned long id_;

  #pragma db version
  unsigned long version_;

  std::string first_;
  std::string last_;
  unsigned short age_;
};

#endif // PERSON_HXX
