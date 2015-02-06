// file      : common/lazy-ptr/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <vector>
#include <string>
#include <memory>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#  include <odb/tr1/lazy-ptr.hxx>
#endif

// Raw pointer.
//
#pragma db namespace table("t1_")
namespace test1
{
  using odb::lazy_ptr;
  class obj;

  #pragma db object
  class cont
  {
  public:
    cont () {}
    cont (unsigned long i): id (i) {}
    ~cont ();

    #pragma db id
    unsigned long id;

    typedef std::vector<lazy_ptr<obj> > obj_list;

    #pragma db value_not_null
    obj_list o;
  };

  #pragma db object
  class obj
  {
  public:
    obj () {}
    obj (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(o) not_null
    lazy_ptr<cont> c; // weak
  };

  inline cont::
  ~cont ()
  {
    for (obj_list::iterator i (o.begin ()); i != o.end (); ++i)
      if (obj* p = i->get ())
        delete p;
  }
}

// std::auto_ptr/std::unique_ptr
//
#pragma db namespace table("t2_")
namespace test2
{
  using odb::lazy_ptr;

  class obj;
  class cont;

#ifdef HAVE_CXX11
  typedef std::unique_ptr<obj> obj_ptr;
  typedef std::unique_ptr<cont> cont_ptr;
  typedef odb::lazy_unique_ptr<obj> lazy_obj_ptr;
#else
  typedef std::auto_ptr<obj> obj_ptr;
  typedef std::auto_ptr<cont> cont_ptr;
  typedef odb::lazy_auto_ptr<obj> lazy_obj_ptr;
#endif

  #pragma db object
  class cont
  {
  public:
    cont () {}
    cont (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db not_null
    lazy_obj_ptr o;
  };

  #pragma db object
  class obj
  {
  public:
    obj () {}
    obj (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db inverse(o) not_null
    lazy_ptr<cont> c; // weak
  };
}

// shared_ptr
//
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
#pragma db namespace table("t3_")
namespace test3
{
#ifdef HAVE_CXX11
  using std::shared_ptr;
  using odb::lazy_shared_ptr;
  using odb::lazy_weak_ptr;
#else
  using std::tr1::shared_ptr;
  using odb::tr1::lazy_shared_ptr;
  using odb::tr1::lazy_weak_ptr;
#endif

  class obj;

  #pragma db object pointer(shared_ptr<cont>)
  class cont
  {
  public:
    cont () {}
    cont (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    typedef std::vector<lazy_weak_ptr<obj> > obj_list;

    #pragma db inverse(c) value_not_null
    obj_list o;
  };

  #pragma db object pointer(shared_ptr<obj>)
  class obj
  {
  public:
    obj () {}
    obj (unsigned long i): id (i) {}

    #pragma db id
    unsigned long id;

    #pragma db not_null
    lazy_shared_ptr<cont> c;
  };
}
#endif

#endif // TEST_HXX
