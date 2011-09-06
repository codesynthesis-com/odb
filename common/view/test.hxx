// file      : common/view/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <memory>
#include <cstddef> // std::size_t
#include <iostream>

#include <odb/core.hxx>

#pragma db object
struct country
{
  country (const std::string& c, std::string const& n)
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

#pragma db object
struct person
{
  person (unsigned long i,
          const std::string& fn,
          const std::string& ln,
          unsigned short a,
          country* l)
      : id (i), first_name (fn), last_name (ln), age (a), location (l)
  {
  }

  ~person ()
  {
    delete location;
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

  #pragma db not_null
  country* location;
};

// General view with a custom query.
//
#pragma db view query("SELECT first, last, age FROM common_view_person")
struct view1
{
  std::string first;
  std::string last;
  unsigned short age;
};

// Count view.
//
#pragma db view query("SELECT count(id) FROM common_view_person")
struct view2
{
  std::size_t count;
};

// Aggregate view.
//
#pragma db view query("SELECT last, count(last) " \
                      "FROM common_view_person "  \
                      "GROUP BY last")
struct view3
{
  std::string last;
  std::size_t count;
};

// JOIN view.
//
#pragma db view query("SELECT first, last, common_view_country.name " \
                      "FROM common_view_person "                      \
                      "LEFT JOIN common_view_country "                \
                      "ON common_view_person.location = "             \
                        "common_view_country.code")
struct view4
{
  std::string first;
  std::string last;
  std::string location;
};

#endif // TEST_HXX
