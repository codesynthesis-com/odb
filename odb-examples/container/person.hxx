// file      : container/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <set>
#include <map>
#include <vector>
#include <string>

#include <odb/core.hxx>

typedef std::vector<std::string> names;
typedef std::set<std::string> emails;
typedef std::map<unsigned short, float> age_weight_map;

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

  // Nicknames.
  //
  const names&
  nicknames () const
  {
    return nicknames_;
  }

  names&
  nicknames ()
  {
    return nicknames_;
  }

  // Emails.
  //
  typedef ::emails emails_type;

  const emails_type&
  emails () const
  {
    return emails_;
  }

  emails_type&
  emails ()
  {
    return emails_;
  }

  // Age-to-weight map.
  //
  const age_weight_map&
  age_weight () const
  {
    return age_weight_;
  }

  age_weight_map&
  age_weight ()
  {
    return age_weight_;
  }

private:
  friend class odb::access;

  person () {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;

  names nicknames_;
  emails_type emails_;
  age_weight_map age_weight_;
};

#endif // PERSON_HXX
