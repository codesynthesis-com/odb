// file      : common/as/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <map>
#include <vector>
#include <string>
#include <memory>  // shared_ptr
#include <utility> // pair, move()
#include <cstddef> // size
#include <cassert>

#include <odb/core.hxx>
#include <odb/vector.hxx>
#include <odb/nullable.hxx>

// @@ Before pushing to master, add '#' to issue number in commit message.
//

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
  operator== (const object& x, const object& y)
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
  operator== (const object& x, const object& y)
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
  operator== (const object& x, const object& y)
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
  operator== (const object& x, const object& y)
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
  operator== (const object& x, const object& y)
  {
    return x.id == y.id && x.v == y.v && x.i == y.i;
  }
}

// Test id type mapping, where the mapped and interface types are not
// implicitly convertible to each other (regression for GH issue #16). Also
// test query column expressions for various type mappings (regression for GH
// issue #31).
//
#pragma db namespace table("t6_")
namespace test6
{
  #pragma db object pointer(std::shared_ptr)
  struct obj1
  {
    obj1 () {}
    obj1 (int i): id (i) {}

    #pragma db id
    int id;
  };

  inline bool
  operator== (const obj1& x, const obj1& y)
  {
    return x.id == y.id;
  }

  #pragma db value
  struct dependency
  {
    dependency () {}

    explicit
    dependency (std::shared_ptr<obj1> o): obj (std::move (o)) {}

    std::shared_ptr<obj1> obj;
  };

  inline bool
  operator== (const dependency& x, const dependency& y)
  {
    return (x.obj == nullptr) == (y.obj == nullptr) &&
           (x.obj == nullptr || *x.obj == *y.obj);
  }

  inline int
  to_int (const std::string& s)
  {
    std::size_t n;
    int r (std::stoi (s, &n));
    assert (n == s.size ());
    return r;
  }

  // Simple interface value type.
  //
  #pragma db namespace table("s_")
  namespace simple_intf
  {
    struct object_id
    {
      object_id (): value_ (0) {}
      object_id (const object_id& o): value_ (o.value_), cache_ (o.cache_) {}

      explicit
      object_id (int v): value_ (v) {}

      explicit
      object_id (const std::string& s): value_ (to_int (s)) {}


      // If the stringified representation is cached for the current or the
      // being assigned from object, then cache the representation for the new
      // value.
      //
      object_id&
      operator= (const object_id& o)
      {
        value_ = o.value_;

        cache_ = !o.cache_.empty () ? o.cache_                :
                 !cache_.empty ()   ? std::to_string (value_) :
                                      "";
        return *this;
      }

      // Cache the stringified representation of the value, if not yet, and
      // return the reference to the cache.
      //
      const std::string&
      string () const
      {
        if (cache_.empty ())
          cache_ = std::to_string (value_);

        return cache_;
      }

      bool operator== (const object_id& v) const {return value_ == v.value_;}
      bool operator!= (const object_id& v) const {return value_ != v.value_;}
      bool operator < (const object_id& v) const {return value_ < v.value_;}

    private:
      int value_;
      mutable std::string cache_;
    };

    #pragma db map type(object_id) as(std::string)  \
      to((?).string ())                             \
      from(test6::simple_intf::object_id (?))

    // @@ TMP Enable when GH issue #32 is fixed and do the same for composite
    //        interface type tests. Also, let's use array of length 2 for
    //        testing.
    //
#if 0
    typedef object_id object_ids[1];

    #pragma db map type(object_ids) as(std::string) \
      to((?)[0].string ())                          \
      from(test6::simple_intf::object_id (?))
#endif

    struct object;

    #pragma db value
    struct next_object
    {
      next_object () {};

      explicit
      next_object (int i): id (i) {}

      #pragma db points_to(object)
      object_id id;
    };

    inline bool
    operator== (const next_object& x, const next_object& y)
    {
      return x.id == y.id;
    }

    #pragma db object pointer(std::shared_ptr)
    struct obj2
    {
      obj2 () {}
      obj2 (const object_id& i): id (i) {}

      #pragma db id
      object_id id;
    };

    inline bool
    operator== (const obj2& x, const obj2& y)
    {
      return x.id == y.id;
    }

    #pragma db value
    struct requirement
    {
      requirement () {}
      requirement (std::shared_ptr<obj2> o): obj (std::move (o)) {}

      std::shared_ptr<obj2> obj;
    };

    inline bool
    operator== (const requirement& x, const requirement& y)
    {
      return (x.obj == nullptr) == (y.obj == nullptr) &&
             (x.obj == nullptr || *x.obj == *y.obj);
    }

    enum class state
    {
      disabled,
      enabled
    };

    inline std::string
    to_string (state s)
    {
      return s == state::disabled ? "disabled" : "enabled";
    }

    inline state
    to_state (const std::string& s)
    {
      assert (s == "disabled" || s == "enabled");
      return s == "disabled" ? state::disabled : state::enabled;
    }

    #pragma db map type(state) as(std::string) \
      to(test6::simple_intf::to_string (?))    \
      from(test6::simple_intf::to_state (?))

    using null_string = odb::nullable<std::string>;
    using null_state  = odb::nullable<state>;
    using null_int    = odb::nullable<int>;

    #pragma db map type(null_state) as(null_string) \
      to((?)                                        \
         ? test6::simple_intf::to_string (*(?))     \
         : test6::simple_intf::null_string ())      \
      from((?)                                      \
           ? test6::simple_intf::to_state (*(?))    \
           : test6::simple_intf::null_state ())

    #pragma db map type(null_int) as(null_string) \
      to((?)                                      \
         ? std::to_string (*(?))                  \
         : test6::simple_intf::null_string ())    \
      from((?)                                    \
           ? test6::to_int (*(?))                 \
           : test6::simple_intf::null_int ())

    // Note that the reason for defining null_object_id as a struct rather
    // than just the odb::nullable<object_id> typedef, is that we want to make
    // sure that the to() expression for the null_object_id type mapping is of
    // a reference type. This way we will be able to test binding of the query
    // params by reference for object members of the null_object_id type.
    //
    struct null_object_id
    {
      null_object_id () {}
      null_object_id (const null_object_id& o)
          : value_ (o.value_), cache_ (o.cache_) {}

      null_object_id (const object_id& v): value_ (v) {}

      explicit
      null_object_id (const null_string& s)
          : value_ (s
                    ? odb::nullable<object_id> (object_id (*s))
                    : odb::nullable<object_id> ()) {}

      // Always cache the representation for the new value.
      //
      null_object_id&
      operator= (const null_object_id& o)
      {
        value_ = o.value_;
        opt_string ();     // Cache stringified value representation.
        return *this;
      }

      // Cache the stringified representation of the value and return the
      // reference to the cache.
      //
      const null_string&
      opt_string () const
      {
        cache_ = value_
                 ? null_string (value_->string ())
                 : null_string ();

        return cache_;
      }

      bool
      operator== (const null_object_id& v) const
      {
        return value_ == v.value_;
      }

    private:
      odb::nullable<object_id> value_;
      mutable null_string cache_;
    };

    #pragma db map type(null_object_id) as(null_string) \
      to((?).opt_string ())                             \
      from(test6::simple_intf::null_object_id (?))

    #pragma db object
    struct object
    {
      object () {}
      object (int i, int r, int n)
          : id (i),
            ref (r),
            //refs{r}, // @@ TMP Enable when GH issue #32 is fixed.
            next (n)
      {
      }

      #pragma db id
      object_id id;

      #pragma db points_to(object)
      object_id ref;

      //object_ids refs; // @@ TMP Enable when GH issue #32 is fixed.

      next_object next;

      dependency  dep;
      requirement req;

      null_string note;
      state usage = state::disabled;

      #pragma db points_to(object)
      null_object_id source;

      null_state source_usage;
      null_int source_distance;
    };

    inline bool
    operator== (const object& x, const object& y)
    {
      return x.id              == y.id           &&
             x.ref             == y.ref          &&
             //x.refs[0] == y.refs[0] && // @@ TMP Enable when GH issue #32 is fixed.
             x.next            == y.next         &&
             x.dep             == y.dep          &&
             x.req             == y.req          &&
             x.note            == y.note         &&
             x.usage           == y.usage        &&
             x.source          == y.source       &&
             x.source_usage    == y.source_usage &&
             x.source_distance == y.source_distance;
    }
  }

  // Composite interface value type.
  //
  #pragma db namespace table("c_")
  namespace composite_intf
  {
    typedef std::pair<std::string, std::string> str_pair;
    #pragma db value(str_pair)

    struct object_id
    {
      object_id (): value1 (0), value2 (0) {}
      object_id (const object_id& o): value1 (o.value1), value2 (o.value2) {}

      object_id (int v1, int v2): value1 (v1), value2 (v2) {}

      explicit
      object_id (const str_pair& sp)
      {
        value1 = to_int (sp.first);
        value2 = to_int (sp.second);
      }

      object_id&
      operator= (const object_id& o)
      {
        value1 = o.value1;
        value2 = o.value2;
        return *this;
      }

      int value1;
      int value2;
    };

    inline str_pair
    to_str_pair (const object_id& i)
    {
      return str_pair (std::to_string (i.value1), std::to_string (i.value2));
    }

    inline bool
    operator== (const object_id& x, const object_id& y)
    {
      return x.value1 == y.value1 && x.value2 == y.value2;
    }

    inline bool
    operator!= (const object_id& x, const object_id& y)
    {
      return x.value1 != y.value1 || x.value2 != y.value2;
    }

    inline bool
    operator< (const object_id& x, const object_id& y)
    {
      return x.value1 != y.value1 ? x.value1 < y.value1 : x.value2 < y.value2;
    }

    #pragma db map type(object_id) as(str_pair)  \
      to(test6::composite_intf::to_str_pair (?)) \
      from(test6::composite_intf::object_id (?))

    using null_str_pair = odb::nullable<str_pair>;
    using null_object_id = odb::nullable<object_id>;

    #pragma db map type(null_object_id) as(null_str_pair) \
      to((?)                                              \
         ? test6::composite_intf::to_str_pair (*(?))      \
         : test6::composite_intf::null_str_pair ())       \
      from((?)                                            \
           ? test6::composite_intf::object_id (*(?))      \
           : test6::composite_intf::null_object_id ())

    struct object;

    #pragma db value
    struct next_object
    {
      next_object () {};
      next_object (int i1, int i2): id (i1, i2) {}

      #pragma db points_to(object)
      object_id id;
    };

    inline bool
    operator== (const next_object& x, const next_object& y)
    {
      return x.id == y.id;
    }

    #pragma db object pointer(std::shared_ptr)
    struct obj2
    {
      obj2 () {}
      obj2 (const object_id& i): id (i) {}

      #pragma db id
      object_id id;
    };

    inline bool
    operator== (const obj2& x, const obj2& y)
    {
      return x.id == y.id;
    }

    #pragma db value
    struct requirement
    {
      requirement () {}
      requirement (std::shared_ptr<obj2> o): obj (std::move (o)) {}

      std::shared_ptr<obj2> obj;
    };

    inline bool
    operator== (const requirement& x, const requirement& y)
    {
      return (x.obj == nullptr) == (y.obj == nullptr) &&
             (x.obj == nullptr || *x.obj == *y.obj);
    }

    #pragma db object
    struct object
    {
      object () {}
      object (int i1, int i2,
              int r1, int r2,
              int n1, int n2)
          : id (i1, i2), ref (r1, r2), next (n1, n2) {}

      #pragma db id
      object_id id;

      #pragma db points_to(object)
      object_id ref;

      next_object next;

      dependency dep;
      requirement req;

      #pragma db points_to(object)
      null_object_id source;
    };

    inline bool
    operator== (const object& x, const object& y)
    {
      return x.id     == y.id   &&
             x.ref    == y.ref  &&
             x.next   == y.next &&
             x.dep    == y.dep  &&
             x.req    == y.req  &&
             x.source == y.source;
    }
  }
}

#endif // TEST_HXX
