// file      : pimpl/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object
class person
{
public:
  ~person ();
  person (const std::string& email,
          const std::string& name,
          unsigned short age);

  const std::string&
  email () const;

  void
  email (const std::string&);

  const std::string&
  name () const;

  void
  name (const std::string&);

  unsigned short
  age () const;

  void
  age (unsigned short) const;

private:
  person (const person&);
  person& operator= (const person&);

private:
  friend class odb::access;
  person ();

  struct impl;

  #pragma db transient
  impl* pimpl_;

  #pragma db member(email) virtual(std::string) id
  #pragma db member(name) virtual(std::string)
  #pragma db member(age) virtual(unsigned short)
};

#endif // PERSON_HXX
