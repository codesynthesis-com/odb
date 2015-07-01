// file      : common/as/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <map>
#include <vector>
#include <string>
#include <utility> // pair

#include <odb/core.hxx>
#include <odb/nullable.hxx>

// Test basic type mapping functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  typedef std::pair<int, int> intp;

  #pragma db value
  struct comp
  {
    comp () {}
    comp (int n1_, int n2_): n1 (n1_), n2 (n2_) {}

    int n1;
    int n2;
  };

  #pragma db map type(intp) as(comp)        \
    to(test1::comp ((?).first, (?).second)) \
    from(test1::intp ((?).n1, (?).n2))

  #pragma db object
  struct object
  {
    // Class-scope mapping.
    //
    #pragma db map type(bool) as(std::string) \
    to((?) ? "true" : "false")                \
    from((?) == "true")

    #pragma db id auto
    unsigned long id;

    bool b;
    intp ip;

    #pragma db transient
    std::map<bool, int> m;

    #pragma db transient
    std::vector<intp> v;

    object () {}
    object (bool b_, int n1, int n2): b (b_), ip (n1, n2) {}
  };

  inline bool
  operator== (const object& x, const object y)
  {
    return x.b == y.b && x.ip == y.ip /*&& x.m == y.m && x.v == y.v*/;
  }
}

// Test wrapped simple type mapping.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db map type(bool) as(std::string) \
    to((?) ? "true" : "false")              \
    from((?) == "true")

  typedef odb::nullable<bool> null_bool;
  typedef odb::nullable<std::string> null_string;

  /*
  #pragma db map type(null_bool) as(null_string)                        \
    to((?)                                                              \
       ? test2::null_string (*(?) ? "true" : "false")                   \
       : test2::null_string ())                                         \
    from((?)                                                            \
         ? test2::null_bool (*(?) == "true")                            \
         : test2::null_bool ())
  */

  #pragma db map type(null_bool) as(std::string)                        \
    to((?) ? (*(?) ? "true" : "false") : "null")                        \
    from((?) != "null"                                                  \
         ? test2::null_bool ((?) == "true")                             \
         : test2::null_bool ())

  #pragma db object
  struct object
  {
    #pragma db id auto
    unsigned long id;

    odb::nullable<bool> b;
  };
}

// Test wrapped simple type mapping.
//
#pragma db namespace table("t3_")
namespace test3
{
  typedef std::pair<int, int> intp;

  #pragma db value
  struct comp
  {
    comp () {}
    comp (int n1_, int n2_): n1 (n1_), n2 (n2_) {}

    int n1;
    int n2;
  };

  typedef odb::nullable<intp> null_intp;
  typedef odb::nullable<comp> null_comp;

  #pragma db map type(null_intp) as(null_comp)                          \
    to((?)                                                              \
       ? test3::null_comp (test3::comp ((?)->first, (?)->second))       \
       : test3::null_comp ())                                           \
    from((?)                                                            \
         ? test3::null_intp (test3::intp ((?)->n1, (?)->n2))            \
         : test3::null_intp ())

  // Map int pair with both members equal 0 to NULL comp.
  //
  #pragma db map type(intp) as(null_comp)                               \
    to((?).first != 0 || (?).second != 0                                \
       ? test3::null_comp (test3::comp ((?).first, (?).second))         \
       : test3::null_comp ())                                           \
    from((?)                                                            \
         ? test3::intp (test3::intp ((?)->n1, (?)->n2))                 \
         : test3::intp (0, 0))

  #pragma db object
  struct object
  {
    #pragma db id auto
    unsigned long id;

    odb::nullable<intp> np;
    intp ip;
  };
}

//@@ Test wrapped id and version.
//@@ Containers.

#endif // TEST_HXX
