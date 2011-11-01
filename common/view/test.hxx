// file      : common/view/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <cstddef> // std::size_t

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include <common/config.hxx> // DATABASE_XXX

struct employer;

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

enum gender_type {male, female};

#pragma db value
struct measures
{
  measures (unsigned short w, unsigned short h) : weight (w), hight (h) {}
  measures () {}

  unsigned short weight;
  unsigned short hight;
};

#pragma db object
struct person
{
  typedef ::measures measures_type;

  person (unsigned long i,
          const std::string& fn,
          const std::string& ln,
          unsigned short a,
          gender_type g,
          const measures_type m,
          country* r,
          country* n)
      : id (i),
        first_name_ (fn),
        last_name_ (ln),
        age (a),
        gender (g),
        measures (m),
        residence (r),
        nationality (n),
        husband (0)
  {
  }

  person ()
  {
  }

  #pragma db id
  unsigned long id;

  #pragma db column("first")
  std::string first_name_;

  #pragma db column("last")
  std::string last_name_;

  unsigned short age;

  // #pragma db type("INT") - in MySQL test type pragma copying
  gender_type gender;

  measures_type measures;

  #pragma db not_null
  country* residence;

  #pragma db not_null
  country* nationality;

  #pragma db inverse(employees)
  employer* employed_by;

  // A non-pointer relationship.
  //
  odb::nullable<std::string> previous_employer;

  person* husband; // Self-reference.
};

#pragma db object
struct employer
{
  employer (const std::string& n)
      : name (n)
  {
  }

  employer ()
  {
  }

  #pragma db id
  std::string name;
  unsigned int head_count;
  std::vector<person*> employees;
};

//
// General view with no associated objects.
//

// Complete suffix query template.
//
#ifndef DATABASE_ORACLE
#  pragma db view query("SELECT first, last, age FROM t_view_person")
#else
#  pragma db view query("SELECT \"first\", \"last\", \"age\" " \
                        "FROM \"t_view_person\"")
#endif
struct view1
{
  std::string first;
  std::string last;
  unsigned short age;
};

// Complete query.
//
#ifndef DATABASE_ORACLE
#  pragma db view query("SELECT first, last, age " \
                        "FROM t_view_person "      \
                        "WHERE age < 31 ORDER BY age")
#else
#  pragma db view query("SELECT \"first\", \"last\", \"age\" " \
                        "FROM \"t_view_person\" "              \
                        "WHERE \"age\" < 31 ORDER BY \"age\"")
#endif
struct view1a
{
  std::string first;
  std::string last;
  unsigned short age;
};

// Complete placeholder query template.
//
#ifndef DATABASE_ORACLE
#  pragma db view query("SELECT first, last, age " \
                        "FROM t_view_person "      \
                        "WHERE age < 31 AND (?) ORDER BY age")
#else
#  pragma db view query("SELECT \"first\", \"last\", \"age\" " \
                        "FROM \"t_view_person\" "              \
                        "WHERE \"age\" < 31 AND (?) ORDER BY \"age\"")
#endif
struct view1b
{
  std::string first;
  std::string last;
  unsigned short age;
};

// Runtime query.
//
#pragma db view //query()
struct view1c
{
  std::string first;
  std::string last;
  unsigned short age;
};

// Assembled SELECT and FROM-lists.
//
#pragma db view table("t_view_person")
struct view1d
{
  #pragma db column("first")
  std::string first;

  #pragma db column("last")
  std::string last;

  #pragma db column("age")
  unsigned short age;
};

//
// Count view plus associated object.
//

// Complete suffix query.
//
#ifndef DATABASE_ORACLE
#  pragma db view object(person) \
  query("SELECT count(id) FROM t_view_person")
#else
#  pragma db view object(person) \
  query("SELECT count(\"id\") FROM \"t_view_person\"")
#endif
struct view2
{
  std::size_t count;
};

// Generated query, literal column.
//
#pragma db view object(person)
struct view2a
{
#ifndef DATABASE_ORACLE
  #pragma db column("count(id)")
#else
  #pragma db column("count(\"id\")")
#endif
  std::size_t count;
};

// Generated query, qualified literal column.
//
#pragma db view object(person)
struct view2b
{
#ifndef DATABASE_ORACLE
  #pragma db column("count(t_view_person.id)")
#else
  #pragma db column("count(\"t_view_person\".\"id\")")
#endif
  std::size_t count;
};

// Generated query, expression column.
//
#pragma db view object(person)
struct view2c
{
  #pragma db column("count(" + person::id + ")")
  std::size_t count;
};

//
// Aggregate view plus associated object with a custom alias.
//

// Complete suffix query.
//
#ifndef DATABASE_ORACLE
#  pragma db view object(person = test) \
  query("SELECT last, count(last) "     \
        "FROM t_view_person "           \
        "GROUP BY last")
#else
#  pragma db view object(person = test)     \
  query("SELECT \"last\", count(\"last\") " \
        "FROM \"t_view_person\" "           \
        "GROUP BY \"last\"")
#endif
struct view3
{
  std::string last_name;
  std::size_t count;
};

// Generated query with integrated query condition and placeholder.
//
#pragma db view object(person = test) \
  query((?) + "GROUP BY" + test::last_name_)
struct view3a
{
  // Automatically resolved to test::last_name_.
  //
  std::string last_name;

  #pragma db column("count(" + test::last_name_ + ")")
  std::size_t count;
};

//
// JOIN view plus associated objects, some with custom aliases.
//

// Complete suffix query.
//
#ifndef DATABASE_ORACLE
#  pragma db view object(person) object(country = residence)  \
  query("SELECT first, last, residence.name "                 \
        "FROM t_view_person "                                 \
        "LEFT JOIN t_view_country AS residence "              \
        "ON t_view_person.residence = residence.code")
#else
#  pragma db view object(person) object(country = residence)   \
  query("SELECT \"first\", \"last\", \"residence\".\"name\" "  \
        "FROM \"t_view_person\" "                              \
        "LEFT JOIN \"t_view_country\" \"residence\" "          \
        "ON \"t_view_person\".\"residence\" = \"residence\".\"code\"")
#endif
struct view4
{
  std::string first_name;
  std::string last_name;
  std::string name;
};

// Generated query.
//
#pragma db view object(person) \
  object(country = residence: person::residence)
struct view4a
{
  std::string first_name;
  std::string last_name;
  std::string name;
};

//
// JOIN the same object twice.
//
#pragma db view object(person) \
  object(country = residence: person::residence) \
  object(country = nationality: person::nationality) \
  query((?) + "ORDER BY" + person::age)
struct view5
{
  std::string first_name;
  std::string last_name;

  #pragma db column(residence::name)
  std::string rname;

  #pragma db column(nationality::name)
  std::string nname;
};

//
// JOIN via one(i)-to-many relationship.
//

// Automatic relationship discovery.
//
#pragma db view object(person) object(employer)
struct view6
{
  std::string first_name;
  std::string last_name;

  #pragma db column(::employer::name)
  std::string employer;
};

// Manual relationship specification, left side.
//
#pragma db view object(person) object(employer: person::employed_by)
struct view6a
{
  std::string first_name;
  std::string last_name;

  #pragma db column(::employer::name)
  std::string employer;
};

// Manual relationship specification, right side.
//
#pragma db view object(person) object(employer: employer::employees)
struct view6b
{
  std::string first_name;
  std::string last_name;

  #pragma db column(::employer::name)
  std::string employer;
};

// The same using tables.
//
#ifndef DATABASE_ORACLE
#  pragma db view table("t_view_person" = "p")                 \
  table("t_view_employer_employees" = "ee": "ee.value = p.id") \
  table("t_view_employer" = "e": "ee.object_id = e.name")
#else
#  pragma db view table("t_view_person" = "p")                                \
  table("t_view_employer_employees" = "ee": "\"ee\".\"value\" = \"p\".\"id\"")\
  table("t_view_employer" = "e": "\"ee\".\"object_id\" = \"e\".\"name\"")
#endif
struct view6c
{
  #pragma db column("p.first")
  std::string first_name;

  #pragma db column("p.last")
  std::string last_name;

  #pragma db column("e"."name")
  std::string employer;
};

//
// JOIN via a custom condition.
//
#pragma db view object(person) \
  object(employer: person::previous_employer == employer::name)\
  query((?) + "ORDER BY" + person::age)
struct view7
{
  std::string first_name;
  std::string last_name;

  odb::nullable<unsigned int> head_count;
};

//
// Self-JOIN.
//
#pragma db view object(person = wife) object(person = husb) \
  query (wife::husband.is_not_null ())
struct view8
{
  #pragma db column(wife::first_name_)
  std::string wife_name;

  #pragma db column(husb::first_name_)
  std::string husb_name;
};

//
// Enum mapping.
//
#pragma db view object(person)
struct view9
{
  std::string first_name;
  std::string last_name;
  gender_type gender;
};

//
// Composite in view.
//
#pragma db view object(person) query((?) + "ORDER BY" + person::age)
struct view10
{
  std::string last_name;
  ::measures measures;
};

//
// Composite in object.
//
#pragma db view object(person) \
  query((person::measures.weight > 60 && person::measures.hight < 190 && (?)) \
        + "ORDER BY" + person::age)
struct view11
{
  std::string last_name;

  #pragma db column(person::measures.hight)
  unsigned short hight;
};

//
// Extract object pointer as object id.
//
#pragma db view object(person)
struct view12
{
  std::string residence;
};

#endif // TEST_HXX
