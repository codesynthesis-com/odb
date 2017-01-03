// file      : common/inheritance/polymorphism/test6.hxx
// copyright : Copyright (c) 2009-2017 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST6_HXX
#define TEST6_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <string>
#include <memory>

#include <odb/core.hxx>
#include <odb/callback.hxx>

// Test polymorphism and callbacks.
//
#pragma db namespace table("t6_")
namespace test6
{
#ifdef HAVE_CXX11
  #pragma db object polymorphic pointer(std::unique_ptr)
#else
  #pragma db object polymorphic pointer(std::auto_ptr)
#endif
  struct root
  {
    virtual ~root () {}
    root (): id (0) {}
    root (unsigned long i, unsigned long n): id (i), num (n) {}

    #pragma db id
    unsigned long id;

    unsigned long num;
  };

  #pragma db object callback(db_callback)
  struct base: root
  {
    base () {}
    base (unsigned long i, unsigned long n, const std::string& s)
        : root (i, n), str (s) {}

    std::string str;

    void
    db_callback (odb::callback_event, odb::database&);

    void
    db_callback (odb::callback_event, odb::database&) const;
  };

  #pragma db object callback(db_callback)
  struct derived: base
  {
    derived () {}
    derived (unsigned long i, unsigned long n, const std::string& s)
        : base (i, n, s), dnum (n + 1), dstr (s + 'd') {}

    unsigned long dnum;
    std::string dstr;

#ifdef HAVE_CXX11
    std::unique_ptr<root> ptr;
#else
    std::auto_ptr<root> ptr;
#endif

    void
    db_callback (odb::callback_event, odb::database&) const;
  };
}

#endif // TEST6_HXX
