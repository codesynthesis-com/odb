// file      : section/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>

#include <odb/core.hxx>
#include <odb/vector.hxx>
#include <odb/section.hxx>

#pragma db object
class person
{
public:
  person (const std::string& first, const std::string& last)
      : first_ (first), last_ (last)
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

  // Biography.
  //
  const std::string&
  bio () const
  {
    return bio_;
  }

  void
  bio (const std::string& bio)
  {
    bio_ = bio;
    extras_.change (); // Mark the section as changed.
  }

  // Nicknames.
  //
  typedef odb::vector<std::string> names;

  const names&
  nicknames () const
  {
    return nicknames_;
  }

  names&
  nicknames ()
  {
    // Because we are using a change-tracking container (odb::vector),
    // ODB will automatically mark the section as changed if we update
    // the nicknames.
    //
    return nicknames_;
  }

  // The extras section accessor and modifier. Notice that they are
  // by-reference.
  //
  const odb::section&
  extras_section () const
  {
    return extras_;
  }

  odb::section&
  extras_section ()
  {
    return extras_;
  }

private:
  friend class odb::access;

  person () {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;

  #pragma db section(extras_)
  std::string bio_;

  #pragma db section(extras_)
  names nicknames_;

  #pragma db load(lazy) update(change)
  odb::section extras_;
};

#endif // PERSON_HXX
