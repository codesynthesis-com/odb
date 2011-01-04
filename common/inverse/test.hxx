// file      : common/inverse/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_TR1_MEMORY

#include <set>
#include <vector>
#include <string>
#include <memory>

#include <odb/core.hxx>

#ifdef HAVE_TR1_MEMORY
#  include <odb/tr1/memory.hxx>
#endif

struct obj1;
struct obj2;
struct obj3;
struct obj4;
struct obj5;

typedef obj1* obj1_ptr;
typedef obj2* obj2_ptr;
typedef obj3* obj3_ptr;
typedef obj4* obj4_ptr;
typedef obj5* obj5_ptr;

typedef std::set<obj1_ptr> obj1_ptr_set;
typedef std::set<obj3_ptr> obj3_ptr_set;
typedef std::set<obj5_ptr> obj5_ptr_set;

#pragma db object
struct obj1
{
  obj1 (): o2 (0), o4 (0) {}
  ~obj1 ();

  #pragma db id
  std::string id;

  obj2_ptr o2;

  #pragma db id_column("obj1_id") value_column("obj3_id")
  obj3_ptr_set o3;

  obj4_ptr o4;

  obj5_ptr_set o5;
};

#pragma db object
struct obj2
{
  #pragma db id auto
  int id;

  std::string str;

  // one-to-one
  //
  #pragma db inverse(o2)
  obj1_ptr o1;
};

#pragma db object
struct obj3
{
  #pragma db id auto
  int id;

  std::string str;

  // one(i)-to-many
  //
  #pragma db inverse (o3)
  obj1_ptr o1;
};

#pragma db object
struct obj4
{
  #pragma db id auto
  int id;

  std::string str;

  // many(i)-to-one
  //
  #pragma db inverse (o4)
  obj1_ptr_set o1;
};

#pragma db object
struct obj5
{
  #pragma db id auto
  int id;

  std::string str;

  // many(i)-to-many
  //
  #pragma db inverse (o5)
  obj1_ptr_set o1;
};

inline obj1::
~obj1 ()
{
  delete o2;
  for (obj3_ptr_set::iterator i (o3.begin ()); i != o3.end (); ++i)
    delete *i;
  delete o4;
  for (obj5_ptr_set::iterator i (o5.begin ()); i != o5.end (); ++i)
    delete *i;
}

// TR1 version
//
#ifdef HAVE_TR1_MEMORY
struct tr1_obj1;
struct tr1_obj2;
struct tr1_obj3;
struct tr1_obj4;
struct tr1_obj5;

typedef std::tr1::shared_ptr<tr1_obj1> tr1_obj1_ptr;
typedef std::tr1::shared_ptr<tr1_obj2> tr1_obj2_ptr;
typedef std::tr1::shared_ptr<tr1_obj3> tr1_obj3_ptr;
typedef std::tr1::shared_ptr<tr1_obj4> tr1_obj4_ptr;
typedef std::tr1::shared_ptr<tr1_obj5> tr1_obj5_ptr;

typedef std::tr1::weak_ptr<tr1_obj1> tr1_obj1_wptr;

typedef std::vector<tr1_obj1_wptr> tr1_obj1_wptr_vec;
typedef std::vector<tr1_obj3_ptr> tr1_obj3_ptr_vec;
typedef std::vector<tr1_obj5_ptr> tr1_obj5_ptr_vec;

#pragma db object pointer(tr1_obj1_ptr)
struct tr1_obj1
{
  #pragma db id
  std::string id;

  tr1_obj2_ptr o2;

  #pragma db id_column("tr1_obj1_id") value_column("tr1_obj3_id")
  tr1_obj3_ptr_vec o3;

  tr1_obj4_ptr o4;
  tr1_obj5_ptr_vec o5;
};

#pragma db object pointer(tr1_obj2_ptr)
struct tr1_obj2
{
  #pragma db id auto
  int id;

  std::string str;

  // one(i)-to-one
  //
  #pragma db inverse(o2)
  tr1_obj1_wptr o1;
};

#pragma db object pointer(tr1_obj3_ptr)
struct tr1_obj3
{
  #pragma db id auto
  int id;

  std::string str;

  // one(i)-to-many
  //
  #pragma db inverse (o3)
  tr1_obj1_wptr o1;
};

#pragma db object pointer(tr1_obj4_ptr)
struct tr1_obj4
{
  #pragma db id auto
  int id;

  std::string str;

  // many(i)-to-one
  //
  #pragma db inverse (o4)
  tr1_obj1_wptr_vec o1;
};

#pragma db object pointer(tr1_obj5_ptr)
struct tr1_obj5
{
  #pragma db id auto
  int id;

  std::string str;

  // many(i)-to-many
  //
  #pragma db inverse (o5)
  tr1_obj1_wptr_vec o1;
};
#endif

#endif // TEST_HXX
