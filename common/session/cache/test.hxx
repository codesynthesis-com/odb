// file      : common/session/cache/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <memory>
#include <vector>

#include <odb/core.hxx>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#endif

// Test the session_required exception.
//
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)

#pragma db namespace table("t1_")
namespace test1
{
#ifdef HAVE_CXX11
  using std::shared_ptr;
  using std::weak_ptr;
#else
  using std::tr1::shared_ptr;
  using std::tr1::weak_ptr;
#endif

  #pragma db namespace(test1) pointer(shared_ptr)

  struct obj2;

  #pragma db object
  struct obj1
  {
    obj1 () {}
    obj1 (unsigned long id): id_ (id) {}

    #pragma db id
    unsigned long id_;

    shared_ptr<obj2> o2;
  };

  #pragma db object
  struct obj2
  {
    obj2 () {}
    obj2 (unsigned long id): id_ (id) {}

    #pragma db id
    unsigned long id_;

    #pragma db inverse (o2)
    std::vector< weak_ptr<obj1> > o1;
  };
}

#endif // HAVE_CXX11 || HAVE_TR1_MEMORY

#endif // TEST_HXX
