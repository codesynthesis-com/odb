// file      : pgsql/stored-proc/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <cstddef> // std::size_t

#include <odb/core.hxx>

#pragma db object
class person
{
public:
  person (unsigned long long id,
          const std::string& first,
          const std::string& last,
          unsigned short age)
      : id_ (id), first_ (first), last_ (last), age_ (age) {}

  const std::string& first () const {return first_;}
  const std::string& last () const  {return last_;}
  unsigned short age () const {return age_;}

private:
  person () {}

  friend class odb::access;

  #pragma db id
  unsigned long long id_;

  std::string first_;
  std::string last_;
  unsigned short age_;
};

// Stored functions.
//
#pragma db view query("/*CALL*/ SELECT * FROM person_range((?))")
struct person_range
{
  unsigned short age;
  std::string first;
  std::string last;
};

#pragma db view query("/*CALL*/ SELECT * FROM person_count()")
struct person_count
{
  unsigned long long count;
};

#pragma db view query("/*CALL*/ SELECT * FROM person_age_increment((?))")
struct person_age_increment {};

// Stored procedures.
//
#pragma db view query("CALL person_age_decrement((?))")
struct person_age_decrement
{
  unsigned short age;
  std::string first;
  std::string last;
};

#pragma db view query("CALL person_age_decrement_void((?))")
struct person_age_decrement_void {};

#endif // TEST_HXX
