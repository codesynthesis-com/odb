// file      : boost/common/smart-ptr/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <odb/core.hxx>
#include <odb/boost/smart-ptr/lazy-ptr.hxx>

struct obj;

using boost::shared_ptr;
using odb::boost::lazy_shared_ptr;
using odb::boost::lazy_weak_ptr;

#pragma db object
struct cont
{
  cont ()
  {
  }

  cont (unsigned long id)
      : id (id)
  {
  }

  #pragma db id
  unsigned long id;

  typedef std::vector<lazy_weak_ptr<obj> > obj_list;

  #pragma db inverse(c) value_not_null
  obj_list o;
};

#pragma db object
struct obj
{
  obj ()
  {
  }

  obj (unsigned long id)
      : id (id)
  {
  }

  #pragma db id
  unsigned long id;

  #pragma db not_null
  lazy_shared_ptr<cont> c;
};

// Test shared_ptr as a value wrapper.
//
#pragma db object
struct obj2
{
  obj2 ()
  {
  }

  obj2 (unsigned long id)
      : id (id)
  {
  }

  #pragma db id
  unsigned long id;

  #pragma db null
  shared_ptr<std::string> str;
};

#endif // TEST_HXX
