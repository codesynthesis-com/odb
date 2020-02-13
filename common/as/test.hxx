// file      : common/as/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <map>
#include <vector>
#include <string>
#include <utility> // pair

#include <odb/core.hxx>
#include <odb/vector.hxx>
#include <odb/nullable.hxx>

// Test basic type mapping functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  enum color {red, green, blue};

  inline const char*
  color_to_string (color c)
  {
    return c == red ? "RED" : (c == green ? "GREEN" : "BLUE");
  }

  inline color
  string_to_color (const std::string& s)
  {
    return s == "RED" ? red : (s == "GREEN" ? green : blue);
  }

  #pragma db map type(color) as(std::string) \
    to(test1::color_to_string (?))           \
    from(test1::string_to_color (?))

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
      to((?) ? "true" : "false")              \
      from((?) == "true")

    #pragma db id auto
    unsigned long id;

    bool b;
    color c;
    intp ip;

    std::map<bool, int> m;
    std::vector<intp> v;
    odb::vector<color> cv;

    object () {}
    object (bool b_, color c_, int n1, int n2): b (b_), c (c_), ip (n1, n2) {}
  };

  inline bool
  operator== (const object& x, const object y)
  {
    return
      x.b == y.b &&
      x.c == y.c &&
      x.ip == y.ip &&
      x.m == y.m &&
      x.v == y.v &&
      x.cv == y.cv;
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
    std::vector<odb::nullable<bool> > v;
  };

  inline bool
  operator== (const object& x, const object y)
  {
    return x.b == y.b && x.v == y.v;
  }
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

    std::vector<odb::nullable<intp> > npv;
    odb::vector<intp> ipv;
  };

  inline bool
  operator== (const object& x, const object y)
  {
    return x.np == y.np && x.ip == y.ip && x.npv == y.npv && x.ipv == y.ipv;
  }
}

// Test id type mapping.
//
struct id_type
{
  typedef unsigned long value_type;
  value_type value;

  id_type (value_type v = 0): value (v) {}
  operator value_type () const {return value;}
};

#pragma db map type(id_type) as(id_type::value_type)

#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object
  struct object
  {
    #pragma db id auto
    id_type id;

    int i;
    odb::vector<int> v;

    object () {}
    object (int i_): i (i_) {}
  };

  inline bool
  operator== (const object& x, const object y)
  {
    return x.id == y.id && x.i == y.i && x.v == y.v;
  }
}

// Test version type mapping.
//
#pragma db namespace table("t5_")
namespace test5
{
  struct version_type
  {
    typedef unsigned short value_type;
    value_type value;

    version_type (value_type v = 0): value (v) {}
    operator value_type () const {return value;}
    version_type& operator++ () {value++; return *this;}
  };

  #pragma db map type(version_type) as(id_type::value_type)

  #pragma db object optimistic
  struct object
  {
    #pragma db id
    id_type id;

    #pragma db version
    version_type v;

    int i;

    object () {}
    object (id_type id_, int i_): id (id_), i (i_) {}
  };

  inline bool
  operator== (const object& x, const object y)
  {
    return x.id == y.id && x.v == y.v && x.i == y.i;
  }
}

#endif // TEST_HXX
