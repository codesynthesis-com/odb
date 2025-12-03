// file      : common/relationship/direct-load/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <map>
#include <vector>
#include <string>
#include <memory>

#include <odb/core.hxx>
#include <odb/vector.hxx>
#include <odb/section.hxx>

// Test unidirectional, non-inverse, pointer as container element.
//
#pragma db namespace table("t1_") pointer(std::shared_ptr)
namespace test1
{
  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* n = ""): id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db direct_load
    std::vector<std::shared_ptr<person>> employees;
  };
}

// Test many-to-one, inverse.
//
#pragma db namespace table("t2_") pointer(std::shared_ptr) session
namespace test2
{
  struct employer;

  #pragma db object
  struct person
  {
    person (unsigned long i = 0,
            const char* n = "",
            std::shared_ptr<test2::employer> e = nullptr)
        : id (i), name (n), employer (e) {}

    #pragma db id
    unsigned long id;

    std::string name;

    std::weak_ptr<test2::employer> employer;
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(employer) direct_load
    std::vector<std::shared_ptr<person>> employees;
  };
}

// Test many-to-many, non-inverse, weak_ptr, both direct load.
//
#pragma db namespace table("t3_") pointer(std::shared_ptr) session
namespace test3
{
  struct employer;

  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* n = "")
        : id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;

    #pragma db direct_load
    std::vector<std::weak_ptr<employer>> employers;

    std::vector<std::string> data; // Test container in delayed loading.
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(employers) direct_load
    std::vector<std::shared_ptr<person>> employees;

    std::vector<std::string> data; // Test container in delayed loading.
  };
}

// Test composite pointed-to object id.
//
#pragma db namespace table("t4_") pointer(std::shared_ptr)
namespace test4
{
  #pragma db value
  struct person_id
  {
    unsigned long i;
    std::string s;
  };

  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* s = "", const char* n = "")
        : id {i, s}, name (n) {}

    #pragma db id
    person_id id;

    std::string name;
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db direct_load
    std::vector<std::shared_ptr<person>> employees;
  };
}

// Test pointers in composite.
//
// Also test custom accessors.
//
#pragma db namespace table("t5_") pointer(std::shared_ptr) session
namespace test5
{
  struct employer;
  struct team;

  #pragma db object
  struct person
  {
    person (unsigned long i = 0,
            const char* n = "",
            std::shared_ptr<test5::team> t = nullptr)
        : id (i), name (n), team (t) {}

    #pragma db id
    unsigned long id;

    std::string name;

    std::shared_ptr<test5::team> team;
  };

  #pragma db object
  struct team
  {
    team (unsigned long i = 0,
          const char* n = "",
          std::shared_ptr<test5::employer> e = nullptr)
        : id (i), name (n), employer (e) {}

    #pragma db id
    unsigned long id;

    std::string name;

    std::shared_ptr<test5::employer> employer;
  };

  #pragma db value
  struct employee
  {
    std::string title;

    #pragma db direct_load
    std::shared_ptr<test5::person> person;

    #pragma db direct_load get(get_team) set (set_team)
    std::weak_ptr<test5::team> team;

    const std::weak_ptr<test5::team>& get_team () const {return team;}
    void set_team (const std::shared_ptr<test5::team>& p) {team = p;}
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    std::vector<employee> employees;
  };
}

// Test smart container.
//
#pragma db namespace table("t6_") pointer(std::shared_ptr)
namespace test6
{
  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* n = ""): id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db direct_load
    odb::vector<std::shared_ptr<person>> employees;
  };
}

// Test container in composite value.
//
#pragma db namespace table("t7_") pointer(std::shared_ptr)
namespace test7
{
  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* n = ""): id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;
  };

  #pragma db value
  struct human_resources
  {
    #pragma db direct_load
    std::vector<std::shared_ptr<person>> employees;
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    human_resources hr;
  };
}

// Test sections in pointed-to object.
//
#pragma db namespace table("t8_") pointer(std::shared_ptr)
namespace test8
{
  #pragma db object
  struct person
  {
    person (unsigned long i = 0,
            const char* f = "",
            const char* s = "")
        : id (i), fname (f), sname (s) {}

    #pragma db id
    unsigned long id;

    #pragma db load(eager) update(change)
    odb::section fname_s;

    #pragma db section(fname_s)
    std::string fname;

    #pragma db load(lazy) update(always)
    odb::section sname_s;

    #pragma db section(sname_s)
    std::string sname;
  };

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db direct_load
    std::vector<std::shared_ptr<person>> employees;
  };
}

// Test direct load of map key.
//
#pragma db namespace table("t9_") pointer(std::shared_ptr)
namespace test9
{
  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* n = ""): id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;
  };

  using employees_map = std::map<std::shared_ptr<person>, std::string>;

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db direct_load
    employees_map employees;
  };
}

// Test direct load of map key and value.
//
#pragma db namespace table("t10_") pointer(std::shared_ptr) session
namespace test10
{
  #pragma db object
  struct person
  {
    person (unsigned long i = 0, const char* n = ""): id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;
  };

  #pragma db object
  struct team
  {
    team (unsigned long i = 0, const char* n = "")
        : id (i), name (n) {}

    #pragma db id
    unsigned long id;

    std::string name;
  };

  using employees_map = std::map<std::shared_ptr<person>,
                                 std::shared_ptr<team>>;

  #pragma db object
  struct employer
  {
    employer (unsigned long i = 0): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db direct_load
    employees_map employees;
  };
}

#endif // TEST_HXX
