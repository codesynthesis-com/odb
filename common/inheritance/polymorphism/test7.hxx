// file      : common/inheritance/polymorphism/test7.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST7_HXX
#define TEST7_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <string>
#include <memory>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#endif

#include <odb/core.hxx>

// Test polymorphism and object cache (session).
//
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)
#pragma db namespace table("t7_")
namespace test7
{
#ifdef HAVE_CXX11
  using std::shared_ptr;
#else
  using std::tr1::shared_ptr;
#endif

  #pragma db object polymorphic pointer(shared_ptr) session
  struct root
  {
    virtual ~root () {}
    root (): id (0) {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
  };

  #pragma db object
  struct base: root
  {
    base () {}
    base (unsigned long i, unsigned long n, const std::string& s)
        : root (i, n), str (s) {}

    std::string str;
  };

  #pragma db object
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;
  };
}
#endif

#endif // TEST7_HXX
