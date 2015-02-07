// file      : common/relationship-query/query/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)

#include <string>

#include <odb/core.hxx>

#ifdef HAVE_CXX11
#  include <memory>
using std::shared_ptr;
#else
#  include <odb/tr1/memory.hxx>
using std::tr1::shared_ptr;
#endif

struct country;

#pragma db value
struct residence_info
{
  residence_info (bool p, shared_ptr<country> l)
      : permanent (p), location (l)
  {
  }

  residence_info ()
  {
  }

  bool permanent;

  #pragma db not_null
  shared_ptr<country> location;
};

#pragma db object pointer(shared_ptr)
struct person
{
  person (unsigned long i,
          const std::string& fn,
          const std::string& ln,
          unsigned short a,
          shared_ptr<country> r,
          bool p,
          shared_ptr<country> n)
      : id (i),
        first_name (fn),
        last_name (ln),
        age (a),
        residence (p, r),
        nationality (n)
  {
  }

  person ()
  {
  }

  #pragma db id
  unsigned long id;

  #pragma db column ("first")
  std::string first_name;

  #pragma db column ("last")
  std::string last_name;

  unsigned short age;

  residence_info residence;

  #pragma db not_null
  shared_ptr<country> nationality;

  shared_ptr<person> husband; // Self-join.
};

struct employer;

#pragma db object pointer(shared_ptr)
struct employee: person
{
  employee (unsigned long i,
          const std::string& fn,
          const std::string& ln,
          unsigned short a,
          shared_ptr<country> r,
          bool p,
          shared_ptr<country> n,
          shared_ptr<employer> e)
      : person (i, fn, ln, a, r, p, n),
        employed_by (e)
  {
  }

  employee ()
  {
  }

  shared_ptr<employer> employed_by;
};

#pragma db object pointer(shared_ptr)
struct employer
{
  employer (const std::string& n, shared_ptr<country> nat)
      : name (n), nationality (nat)
  {
  }

  employer ()
  {
  }

  #pragma db id
  std::string name;

  // The same member name and type as in person (test JOIN alias).
  //
  #pragma db not_null
  shared_ptr<country> nationality;
};

#pragma db object pointer(shared_ptr)
struct country
{
  country (const std::string& c, const std::string& n)
      : code (c), name (n)
  {
  }

  country ()
  {
  }

  #pragma db id
  std::string code; // ISO 2-letter country code.

  std::string name;
};

#endif // HAVE_CXX11 || HAVE_TR1_MEMORY
#endif // TEST_HXX
