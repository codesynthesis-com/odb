// file      : mapping/person.hxx
// copyright : not copyrighted - public domain

#ifndef PERSON_HXX
#define PERSON_HXX

#include <string>
#include <ostream>

#include <odb/core.hxx>

// In our database boolean values are stored as strings with valid
// values being "true" and "false".
//
#pragma db value(bool) type("VARCHAR(5)")

#pragma db value type("DATE")
class date
{
public:
  date (unsigned int year, unsigned int month, unsigned int day)
      : year_ (year), month_ (month), day_ (day)
  {
  }

  unsigned int
  year () const
  {
    return year_;
  }

  unsigned int
  month () const
  {
    return month_;
  }

  unsigned int
  day () const
  {
    return day_;
  }

private:
  unsigned int year_;
  unsigned int month_;
  unsigned int day_;
};

inline std::ostream&
operator<< (std::ostream& os, const date& d)
{
  return os << d.year () << '-' << d.month () << '-' << d.day ();
}

#pragma db object
class person
{
public:
  person (const std::string& first,
          const std::string& last,
          const date& born,
          bool married)
      : first_ (first), last_ (last), born_ (born), married_ (married)
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

  const date&
  born () const
  {
    return born_;
  }

  bool
  married () const
  {
    return married_;
  }

private:
  friend class odb::access;

  person (): born_ (0, 0, 0) {}

  #pragma db id auto
  unsigned long id_;

  std::string first_;
  std::string last_;
  date born_;
  bool married_;
};

#endif // PERSON_HXX
