// file      : common/composite/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>
#include <utility> // std::pair

#include <odb/core.hxx>

// Test basic composite functionality.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db value
  struct name
  {
    std::string first;
    std::string last;
  };

  #pragma db value
  struct name_title
  {
    std::string title;
  };

  #pragma db value
  struct name_title_ex: name_title
  {
    // Test value types without data members.
  };

  #pragma db value
  struct name_flags
  {
    bool nick;
    bool alias;
  };

  #pragma db value
  struct name_ex: name, name_title_ex
  {
    name alias;
    std::string nick;

    #pragma db column("show_")
    name_flags flags;
  };

  #pragma db object
  struct person
  {
    person () {}
    person (unsigned long id): id_ (id) {}

    #pragma db id
    unsigned long id_;

    #pragma db column("")
    name_ex name_;

    unsigned short age_;
  };

  inline bool
  operator== (const person& x, const person& y)
  {
    return x.id_ == y.id_ &&
      x.name_.first ==  y.name_.first&&
      x.name_.last == y.name_.last &&
      x.name_.title == y.name_.title &&
      x.name_.alias.first == y.name_.alias.first &&
      x.name_.alias.last == y.name_.alias.last &&
      x.name_.nick == y.name_.nick &&
      x.name_.flags.nick == y.name_.flags.nick &&
      x.name_.flags.alias == y.name_.flags.alias &&
      x.age_ == y.age_;
  }
}

// Test composite class template instantiation.
//
#pragma db namespace table("t2_")
namespace test2
{
  template <typename I, typename S>
  struct comp
  {
    I num;
    S str;
    std::vector<std::pair<I, S> > vec;
  };

  template <typename I, typename S>
  inline bool
  operator== (const comp<I, S>& x, const comp<I, S>& y)
  {
    return x.num == y.num && x.str == y.str && x.vec == y.vec;
  }

  typedef std::pair<int, std::string> int_str_pair;
  #pragma db value(int_str_pair)

  // Make sure we use the name that was specified in the pragma.
  //
#ifdef ODB_COMPILER
  typedef comp<int, std::string> int_str_comp1;
#endif

  typedef comp<int, std::string> int_str_comp;
  #pragma db value(int_str_comp)

  #pragma db object
  struct object
  {
    object () {}
    object (unsigned long id): id_ (id) {}

    #pragma db id
    unsigned long id_;

    comp<int, std::string> comp_;
    std::pair<int, std::string> pair_;
    std::vector<int_str_pair> vec_;
  };

  inline bool
  operator== (const object& x, const object& y)
  {
    return x.id_ == y.id_ &&
      x.comp_ == y.comp_ &&
      x.pair_ == y.pair_ &&
      x.vec_ == y.vec_;
  }
}

// Test empty column name.
//
#pragma db namespace table("t3_")
namespace test3
{
  #pragma db value
  struct comp
  {
    #pragma db column("")
    std::string str;
  };

  #pragma db object
  struct object
  {
    object () {}
    object (unsigned long id): id_ (id) {}

    #pragma db id
    unsigned long id_;

    comp c_;
  };

  inline bool
  operator== (const object& x, const object& y)
  {
    return x.id_ == y.id_ && x.c_.str == y.c_.str;
  }
}

// Test composite definition inside object.
//
#pragma db namespace table("t4_")
namespace test4
{
  #pragma db object
  struct object
  {
    object (unsigned long id = 0): id_ (id) {}

    unsigned long id () const {return id_;}

    void str (const std::string& s) {c_.str = s;}
    const std::string& str () const {return c_.str;}

    void x (int i) {p_.first = i;}
    int x () const {return p_.first;}

    void y (int i) {p_.second = i;}
    int y () const {return p_.second;}

  private:
    friend class odb::access;

    #pragma db id
    unsigned long id_;

    #pragma db value
    struct comp
    {
      std::string str;
    };

    comp c_;

    typedef std::pair<int, int> int_pair;
    #pragma db value(int_pair)

    int_pair p_;
  };

  inline bool
  operator== (const object& x, const object& y)
  {
    return x.id () == y.id () && x.str () == y.str () &&
      x.x () == y.x () && x.y () == y.y ();
  }
}

// Test composite name clashes in query columns (compilation test)
//
#pragma db namespace table("t5_")
namespace test5
{
  // Class-member conflict.
  //
  #pragma db value
  struct value {int value_;};

  // Class-class conflict.
  //
  #pragma db value
  struct inner {int value;};

  #pragma db value
  struct outer {inner value;};

  #pragma db object
  struct object
  {
    #pragma db id
    int id;

    outer value;
    test5::value v;
  };
}

#endif // TEST_HXX
