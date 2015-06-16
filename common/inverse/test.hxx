// file      : common/inverse/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11, HAVE_TR1_MEMORY

#include <set>
#include <vector>
#include <string>
#include <memory>

#include <odb/core.hxx>

#if !defined(HAVE_CXX11) && defined(HAVE_TR1_MEMORY)
#  include <odb/tr1/memory.hxx>
#endif

// Test raw pointers.
//
#pragma db namespace table("t1_")
namespace test1
{
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

    // one-to-one
    //
    #pragma db inverse(o2)
    obj1_ptr o1;

    std::string str;
  };

  #pragma db object
  struct obj3
  {
    std::string str;

    // one(i)-to-many
    //
    #pragma db inverse (o3)
    obj1_ptr o1;

    #pragma db id auto
    int id;
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
}

// Test shared_ptr/weak_ptr.
//
#if defined(HAVE_CXX11) || defined(HAVE_TR1_MEMORY)

#pragma db namespace table("t2_")
namespace test2
{
#ifdef HAVE_CXX11
  using std::shared_ptr;
  using std::weak_ptr;
#else
  using std::tr1::shared_ptr;
  using std::tr1::weak_ptr;
#endif

  struct obj1;
  struct obj2;
  struct obj3;
  struct obj4;
  struct obj5;

  typedef shared_ptr<obj1> obj1_ptr;
  typedef shared_ptr<obj2> obj2_ptr;
  typedef shared_ptr<obj3> obj3_ptr;
  typedef shared_ptr<obj4> obj4_ptr;
  typedef shared_ptr<obj5> obj5_ptr;

  typedef weak_ptr<obj1> obj1_wptr;

  typedef std::vector<obj1_wptr> obj1_wptr_vec;
  typedef std::vector<obj3_ptr> obj3_ptr_vec;
  typedef std::vector<obj5_ptr> obj5_ptr_vec;

  #pragma db object pointer(obj1_ptr)
  struct obj1
  {
    #pragma db id
    std::string id;

    obj2_ptr o2;

    #pragma db id_column("obj1_id") value_column("obj3_id")
    obj3_ptr_vec o3;

    obj4_ptr o4;
    obj5_ptr_vec o5;
  };

  #pragma db object pointer(obj2_ptr)
  struct obj2
  {
    #pragma db id auto
    int id;

    std::string str;

    // one(i)-to-one
    //
    #pragma db inverse(o2)
    obj1_wptr o1;
  };

  #pragma db object pointer(obj3_ptr)
  struct obj3
  {
    #pragma db id auto
    int id;

    std::string str;

    // one(i)-to-many
    //
    #pragma db inverse (o3)
    obj1_wptr o1;
  };

  #pragma db object pointer(obj4_ptr)
  struct obj4
  {
    #pragma db id auto
    int id;

    std::string str;

    // many(i)-to-one
    //
    #pragma db inverse (o4)
    obj1_wptr_vec o1;
  };

  #pragma db object pointer(obj5_ptr)
  struct obj5
  {
    #pragma db id auto
    int id;

    std::string str;

    // many(i)-to-many
    //
    #pragma db inverse (o5)
    obj1_wptr_vec o1;
  };
}
#endif

// Test inverse based on points_to.
//
#pragma db namespace table("t3_")
namespace test3
{
  // Inverse pointer.
  //
  #pragma db value
  struct comp
  {
    int i;
    int j;
  };

  inline bool
  operator< (comp x, comp y) {return x.i < y.i || (x.i == y.i && x.j < y.j);}

  struct obj2;

  #pragma db object
  struct obj1
  {
    #pragma db id
    comp id;

    #pragma db inverse(o1)
    obj2* o2;

    obj1 (int i = 0, int j = 0): o2 (0) {id.i = i; id.j = j;}
    ~obj1 ();
  };

  #pragma db object
  struct obj2
  {
    #pragma db id auto
    int id;

    #pragma db points_to(obj1) on_delete(cascade)
    comp o1;
  };

  inline obj1::
  ~obj1 () {delete o2;}

  // Inverse container of pointers.
  //
  struct obj4;

  #pragma db object
  struct obj3
  {
    #pragma db id auto
    int id;

    #pragma db inverse(o3)
    std::vector<obj4*> o4;

    ~obj3 ();
  };

  #pragma db object
  struct obj4
  {
    #pragma db id auto
    int id;

    #pragma db points_to(obj3)
    int o3;
  };

  inline obj3::
  ~obj3 ()
  {
    for (std::vector<obj4*>::iterator i (o4.begin ()); i != o4.end (); ++i)
      delete *i;
  }
};

// Test inverse with nested data members.
//
#pragma db namespace table("t4_")
namespace test4
{
  // Inverse pointer.
  //
  struct obj1;
  struct obj2;

  #pragma db value
  struct obj2_id
  {
    #pragma db points_to(obj1)
    int i;
    int j;
  };

  inline bool
  operator< (obj2_id x, obj2_id y)
  {return x.i < y.i || (x.i == y.i && x.j < y.j);}

  #pragma db object
  struct obj1
  {
    #pragma db id auto
    int id;

    #pragma db inverse(id.i)
    obj2* o2;

    obj1 (): o2 (0) {}
    ~obj1 ();
  };

  #pragma db object
  struct obj2
  {
    #pragma db id
    obj2_id id;
  };

  inline obj1::
  ~obj1 () {delete o2;}

  // Inverse container of pointers.
  //
  struct obj3;
  struct obj4;

  #pragma db value
  struct obj4_id
  {
    #pragma db points_to(obj3)
    int i;
    int j;
  };

  inline bool
  operator< (obj4_id x, obj4_id y)
  {return x.i < y.i || (x.i == y.i && x.j < y.j);}

  #pragma db object
  struct obj3
  {
    #pragma db id auto
    int id;

    #pragma db inverse(id.i)
    std::vector<obj4*> o4;

    ~obj3 ();
  };

  #pragma db object
  struct obj4
  {
    #pragma db id
    obj4_id id;
  };

  inline obj3::
  ~obj3 ()
  {
    for (std::vector<obj4*>::iterator i (o4.begin ()); i != o4.end (); ++i)
      delete *i;
  }
};
#endif // TEST_HXX
