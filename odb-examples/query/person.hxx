// file      : query/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

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

  person (const std::string& first,
          const std::string& middle,
          const std::string& last,
          unsigned short age)
      : first_ (first), middle_ (middle), last_ (last), age_ (age)
  {
  }

  const std::string&
  first () const
  {
    return first_;
  }

  const odb::nullable<std::string>&
  middle () const
  {
    return middle_;
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
  odb::nullable<std::string> middle_;
  std::string last_;
  unsigned short age_;
};

#endif // PERSON_HXX
