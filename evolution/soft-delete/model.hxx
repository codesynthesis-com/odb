// file      : evolution/soft-delete/model.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <string>
#include <memory> // std::auto_ptr

#include <odb/core.hxx>
#include <odb/vector.hxx>
#include <odb/section.hxx>
#include <odb/lazy-ptr.hxx>

#pragma db model version(1, MODEL_VERSION)

#define MODEL_NAMESPACE_IMPL(V) v##V
#define MODEL_NAMESPACE(V) MODEL_NAMESPACE_IMPL(V)

namespace MODEL_NAMESPACE(MODEL_VERSION)
{
  // Test soft-deleted objects.
  //
  #pragma db namespace table("t1_")
  namespace test1
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db object(object) deleted(3)
#endif
  }

  // Test basic soft-deleted member logic.
  //
  #pragma db namespace table("t2_")
  namespace test2
  {
    struct object;

    #pragma db object
    struct object1
    {
      object1 (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      odb::vector<odb::lazy_ptr<object> > ptrs;
    };

    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id), ptr (0) {}
      ~object () {delete ptr;}

      #pragma db id
      unsigned long id_;

      std::string str;
      unsigned long num;
      odb::vector<int> vec;

      #pragma db inverse(ptrs)
      object1* ptr;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
    #pragma db member(object::vec) deleted(3)
    #pragma db member(object::ptr) deleted(3)
#endif
  }

  // Test empty statement handling (INSERT, UPDATE).
  //
  #pragma db namespace table("t3_")
  namespace test3
  {
    #pragma db object
    struct object
    {
      #pragma db id auto
      unsigned long id;

      std::string str;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test empty statement handling (SELECT in polymorphic loader).
  //
  #pragma db namespace table("t4_")
  namespace test4
  {
    #pragma db object polymorphic
    struct base
    {
      virtual
      ~base () {}
      base (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;
    };

    #pragma db object
    struct object: base
    {
      object (unsigned long id = 0): base (id) {}

      std::string str;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test container with soft-deleted value member.
  //
  #pragma db namespace table("t5_")
  namespace test5
  {
    #pragma db value
    struct value
    {
      value () {}
      value (const std::string& s, unsigned long n): str (s), num (n) {}

      std::string str;
      unsigned long num;
    };

    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      odb::vector<value> vec;
    };

#if MODEL_VERSION == 3
    #pragma db member(value::str) deleted(3)
#endif
  }

  // Test view with soft-deleted member.
  //
  #pragma db namespace table("t6_")
  namespace test6
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      std::string str;
      unsigned long num;
    };

    #pragma db view object(object)
    struct view
    {
      std::string str;
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
    #pragma db member(view::str) deleted(3)
#endif
  }

  // Test soft-deleted section member.
  //
  #pragma db namespace table("t7_")
  namespace test7
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      #pragma db load(lazy) update(change)
      odb::section s;

      #pragma db section(s)
      std::string str;

      unsigned long num;

      #pragma db section(s)
      odb::vector<int> vec;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::s) deleted(3)
#endif
  }

  // Test soft-deleted members of a section.
  //
  #pragma db namespace table("t8_")
  namespace test8
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      #pragma db load(lazy) update(change)
      odb::section s;

      #pragma db section(s)
      std::string str;

      #pragma db section(s)
      unsigned long num;

      #pragma db section(s)
      odb::vector<int> vec;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
    #pragma db member(object::vec) deleted(3)
#endif
  }

  // Test basic soft-deleted member logic in polymorphic classes.
  //
  #pragma db namespace table("t9_")
  namespace test9
  {
    #pragma db object polymorphic
    struct base
    {
      virtual
      ~base () {}
      base (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      std::string bstr;
    };

    #pragma db object
    struct object: base
    {
      object (unsigned long id = 0): base (id) {}

      std::string dstr;
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(base::bstr) deleted(3)
    #pragma db member(object::dstr) deleted(3)
#endif
  }

  // Test soft-deleted section member in polymorphic classes.
  //
  #pragma db namespace table("t10_")
  namespace test10
  {
    #pragma db object polymorphic
    struct base
    {
      virtual
      ~base () {}
      base (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      #pragma db load(lazy) update(change)
      odb::section s;

      #pragma db section(s)
      std::string bstr;
    };

    #pragma db object
    struct object: base
    {
      object (unsigned long id = 0): base (id) {}

      #pragma db section(s)
      std::string dstr;

      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(base::s) deleted(3)
#endif
  }

  // Test soft-deleted members of a section in polymorphic classes.
  //
  #pragma db namespace table("t11_")
  namespace test11
  {
    #pragma db object polymorphic
    struct base
    {
      virtual
      ~base () {}
      base (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      #pragma db load(lazy) update(change)
      odb::section s;

      #pragma db section(s)
      std::string bstr;
    };

    #pragma db object
    struct object: base
    {
      object (unsigned long id = 0): base (id) {}

      #pragma db section(s)
      std::string dstr;

      #pragma db section(s)
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(base::bstr) deleted(3)
    #pragma db member(object::dstr) deleted(3)
#endif
  }

  // Test soft-deleted member and optimistic concurrency.
  //
  #pragma db namespace table("t12_")
  namespace test12
  {
    #pragma db object optimistic
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      #pragma db version mssql:type("ROWVERSION")
      unsigned long long v_;

      std::string str;
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test soft-deleted member in an object without id.
  //
  #pragma db namespace table("t13_")
  namespace test13
  {
    #pragma db object no_id
    struct object
    {
      std::string str;
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test soft-deleted member in an object with auto id.
  //
  #pragma db namespace table("t14_")
  namespace test14
  {
    #pragma db object
    struct object
    {
      std::string str;
      unsigned long num;

      #pragma db id auto
      unsigned long id;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::str) deleted(3)
#endif
  }

  // Test summarily deleted composite values.
  //
  #pragma db namespace table("t15_")
  namespace test15
  {
    #pragma db value
    struct value
    {
      std::string str;
      odb::vector<int> vec;
    };

    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      std::auto_ptr<value> v;
      unsigned long num;
    };

#if MODEL_VERSION == 3
    #pragma db member(value::str) deleted(3)
    #pragma db member(value::vec) deleted(3)
#endif
  }

  // Test soft-deleted container member in a non-versioned object.
  //
  #pragma db namespace table("t21_")
  namespace test21
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      unsigned long num;
      odb::vector<int> vec;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::vec) deleted(3)
#endif
  }

  // Test soft-deleted container member in a non-versioned section.
  //
  #pragma db namespace table("t22_")
  namespace test22
  {
    #pragma db object
    struct object
    {
      object (unsigned long id = 0): id_ (id) {}

      #pragma db id
      unsigned long id_;

      #pragma db load(lazy) update(change)
      odb::section s;

      #pragma db section(s)
      unsigned long num;

      #pragma db section(s)
      odb::vector<int> vec;
    };

#if MODEL_VERSION == 3
    #pragma db member(object::vec) deleted(3)
#endif
  }
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
