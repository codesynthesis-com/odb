// file      : access/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object
class person
{
public:
  person () {}
  person (const std::string& email,
          const std::string& first,
          const std::string& middle,
          const std::string& last,
          unsigned short age)
      : email_ (email), first_ (first), middle_ (middle), last_ (last)
  {
    data_.age = age;
  }

  // Standard accessor/modifier names. Auto-discovered by ODB.
  //
  const std::string&
  email () const
  {
    return email_;
  }

  void
  email (const std::string& email)
  {
    email_ = email;
  }

  // Get/set-style accessor/modifier names. Also auto-discovered
  // by ODB.
  //
  const std::string&
  getFirst () const
  {
    return first_;
  }

  std::string&
  setFirst ()
  {
    return first_;
  }

  // Unconventional accessor/modifier names which ODB is unable to
  // auto-discover (but see also the --{accessor,modifier}-regex
  // options). We have to specify these names explicitly (see below).
  //
  const std::string&
  g_middle () const
  {
    return middle_;
  }

  void
  s_middle (const std::string& middle)
  {
    middle_ = middle;
  }

  // Accessor/modifier types do not match data member type. Again,
  // we have to specify accessor/modifier expressions that perform
  // the necessary conversions (see below).
  //
  const char*
  last () const
  {
    return last_.c_str ();
  }

  void
  last (const char* last)
  {
    last_ = last;
  }

  // Accessor/modifier for a data member that is wrapped in an
  // anonymous struct. We use a virtual data member to handle
  // this case.
  //
  unsigned short
  age () const
  {
    return data_.age;
  }

  void
  age (unsigned short age)
  {
    data_.age = age;
  }

private:
  #pragma db id
  std::string email_; // Accessor and modifier are auto-discovered.

  std::string first_; // Accessor and modifier are auto-discovered.

  #pragma db get(g_middle) set(s_middle)
  std::string middle_;

  #pragma db get(std::string (this.last ())) set(last ((?).c_str ()))
  std::string last_;

  #pragma db transient
  struct
  {
    unsigned short age;
  } data_;

  #pragma db member(age) virtual(unsigned short) // Accessor and modifier
                                                 // are auto-discovered.
};

#endif // PERSON_HXX
