// file      : common/lazy-ptr/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_TR1_MEMORY

#include <vector>
#include <string>
#include <memory>

#include <odb/core.hxx>
#include <odb/lazy-ptr.hxx>

#ifdef HAVE_TR1_MEMORY
#  include <odb/tr1/memory.hxx>
#  include <odb/tr1/lazy-ptr.hxx>
#endif

// Raw pointer.
//
using odb::lazy_ptr;
class obj1;

#pragma db object
class cont1
{
public:
  cont1 () {}
  cont1 (unsigned long i): id (i) {}
  ~cont1 ();

  #pragma db id
  unsigned long id;

  typedef std::vector<lazy_ptr<obj1> > obj_list;

  #pragma db not_null
  obj_list o;
};

#pragma db object
class obj1
{
public:
  obj1 () {}
  obj1 (unsigned long i): id (i) {}

  #pragma db id
  unsigned long id;

  #pragma db inverse(o) not_null
  lazy_ptr<cont1> c; // weak
};

inline cont1::
~cont1 ()
{
  for (obj_list::iterator i (o.begin ()); i != o.end (); ++i)
    if (i->loaded ())
      delete i->get ();
}

// Auto pointer.
//
using std::auto_ptr;
using odb::lazy_auto_ptr;

class obj2;

#pragma db object
class cont2
{
public:
  cont2 () {}
  cont2 (unsigned long i): id (i) {}

  #pragma db id
  unsigned long id;

  #pragma db not_null
  lazy_auto_ptr<obj2> o;
};

#pragma db object
class obj2
{
public:
  obj2 () {}
  obj2 (unsigned long i): id (i) {}

  #pragma db id
  unsigned long id;

  #pragma db inverse(o) not_null
  lazy_ptr<cont2> c; // weak
};

// TR1.
//
#ifdef HAVE_TR1_MEMORY
namespace tr1
{
  using std::tr1::shared_ptr;
  using odb::tr1::lazy_shared_ptr;
  using odb::tr1::lazy_weak_ptr;

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

    #pragma db inverse(c) not_null
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
