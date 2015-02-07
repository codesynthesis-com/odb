// file      : common/composite-id/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

#pragma db value
struct scomp
{
  scomp () {}
  scomp (const std::string& s1, const std::string& s2, const std::string& s3)
      : str1 (s1), str2 (s2), str3 (s3)
  {
  }

  std::string str1;
  std::string str2;
  std::string str3;
};

inline bool
operator== (const scomp& x, const scomp& y)
{
  return x.str1 == y.str1 && x.str2 == y.str2 && x.str3 == y.str3;
}

inline bool
operator< (const scomp& x, const scomp& y)
{
  return x.str1 < y.str1 ||
    (x.str1 == y.str1 && x.str2 < y.str2) ||
    (x.str1 == y.str1 && x.str2 == y.str2 && x.str3 < y.str3);
}

#pragma db value
struct ncomp
{
  ncomp () {}
  ncomp (unsigned short n1, unsigned short n2, unsigned short n3)
      : num1 (n1), num2 (n2), num3 (n3)
  {
  }

  unsigned short num1;
  unsigned short num2;
  unsigned short num3;
};

inline bool
operator== (const ncomp& x, const ncomp& y)
{
  return x.num1 == y.num1 && x.num2 == y.num2 && x.num3 == y.num3;
}

inline bool
operator< (const ncomp& x, const ncomp& y)
{
  return x.num1 < y.num1 ||
    (x.num1 == y.num1 && x.num2 < y.num2) ||
    (x.num1 == y.num1 && x.num2 == y.num2 && x.num3 < y.num3);
}

// Test object with composite id, container.
//
#pragma db namespace table("t1_")
namespace test1
{
  #pragma db object
  struct object
  {
    object () {}
    object (const scomp& i, unsigned long n): id (i), num (n) {}

    #pragma db id
    scomp id;

    unsigned long num;
    std::vector<scomp> vec;
  };

  inline bool
  operator== (const object& x, const object& y)
  {
    return x.id == y.id && x.num == y.num && x.vec == y.vec;
  }
}

// Test to-one and to-many relationships with composite id as well as
// queries and views.
//
#pragma db namespace table("t2_")
namespace test2
{
  #pragma db object
  struct object1
  {
    object1 () {}
    object1 (const scomp& i): id (i) {}

    #pragma db id
    scomp id;
  };

  #pragma db object
  struct object2
  {
    object2 (): o1 (0) {}
    object2 (const ncomp& i): id (i), o1 (0) {}
    ~object2 () {delete o1;}

    #pragma db id
    ncomp id;

    object1* o1;
  };

  #pragma db object
  struct object3
  {
    object3 () {}
    object3 (const ncomp& i): id (i) {}

    ~object3 ()
    {
      for (std::vector<object1*>::iterator i (o1.begin ());
           i != o1.end (); ++i)
        delete *i;
    }

    #pragma db id
    ncomp id;

    std::vector<object1*> o1;
  };

  // Test second-level query pointer test as well as pointers in
  // composite types.
  //
  #pragma db value
  struct comp
  {
    comp (): o2 (0) {}
    ~comp () {delete o2;}

    object2* o2;
  };

  #pragma db object
  struct object4
  {
    object4 () {}
    object4 (const ncomp& i): id (i) {}

    #pragma db id
    ncomp id;

    comp c;
  };

  #pragma db view object(object2) object(object1)
  struct view2
  {
    #pragma db column (object2::id.num3)
    unsigned short num;

    #pragma db column (object1::id.str3)
    std::string str;
  };

  #pragma db view object(object3) object(object1)
  struct view3
  {
    #pragma db column (object3::id.num3)
    unsigned short num;

    #pragma db column (object1::id.str3)
    std::string str;
  };

  #pragma db view object(object4) object(object2) object(object1)
  struct view4
  {
    #pragma db column (object4::id.num3)
    unsigned short num4;

    #pragma db column (object2::id.num3)
    unsigned short num2;

    #pragma db column (object1::id.str3)
    std::string str;
  };
}

// Test one-to-one(i) relationship with composite id.
//
#pragma db namespace table("t3_")
namespace test3
{
  struct object2;

  #pragma db object
  struct object1
  {
    object1 () {}
    object1 (const scomp& i): id (i) {}

    #pragma db id
    scomp id;

    #pragma db inverse(o1)
    object2* o2;
  };

  #pragma db object
  struct object2
  {
    object2 (): o1 (0) {}
    object2 (const ncomp& i): id (i), o1 (0) {}
    ~object2 () {delete o1;}

    #pragma db id
    ncomp id;

    object1* o1;
  };

  #pragma db view object(object2) object(object1)
  struct view
  {
    #pragma db column (object2::id.num3)
    unsigned short num;

    #pragma db column (object1::id.str3)
    std::string str;
  };
}

// Test many-to-one(i) relationship with composite id.
//
#pragma db namespace table("t4_")
namespace test4
{
  struct object2;

  #pragma db object
  struct object1
  {
    object1 () {}
    object1 (const scomp& i): id (i) {}

    #pragma db id
    scomp id;

    #pragma db inverse(o1)
    object2* o2;
  };

  #pragma db object
  struct object2
  {
    object2 () {}
    object2 (const ncomp& i): id (i) {}

    ~object2 ()
    {
      for (std::vector<object1*>::iterator i (o1.begin ());
           i != o1.end (); ++i)
        delete *i;
    }

    #pragma db id
    ncomp id;

    std::vector<object1*> o1;
  };

  #pragma db view object(object2) object(object1)
  struct view
  {
    #pragma db column (object2::id.num3)
    unsigned short num;

    #pragma db column (object1::id.str3)
    std::string str;
  };
}

// Test one-to-many(i) relationship with composite id.
//
#pragma db namespace table("t5_")
namespace test5
{
  struct object2;

  #pragma db object
  struct object1
  {
    object1 () {}
    object1 (const scomp& i): id (i) {}

    #pragma db id
    scomp id;

    object2* o2;
  };

  #pragma db object
  struct object2
  {
    object2 () {}
    object2 (const ncomp& i): id (i) {}

    ~object2 ()
    {
      for (std::vector<object1*>::iterator i (o1.begin ());
           i != o1.end (); ++i)
        delete *i;
    }

    #pragma db id
    ncomp id;

    #pragma db inverse(o2)
    std::vector<object1*> o1;
  };

  #pragma db view object(object2) object(object1)
  struct view
  {
    #pragma db column (object2::id.num3)
    unsigned short num;

    #pragma db column (object1::id.str3)
    std::string str;
  };
}

// Test many-to-many(i) relationship with composite id.
//
#pragma db namespace table("t6_")
namespace test6
{
  struct object2;

  #pragma db object
  struct object1
  {
    object1 () {}
    object1 (const scomp& i): id (i) {}

    #pragma db id
    scomp id;

    std::vector<object2*> o2;
  };

  #pragma db object
  struct object2
  {
    object2 () {}
    object2 (const ncomp& i): id (i) {}

    ~object2 ()
    {
      for (std::vector<object1*>::iterator i (o1.begin ());
           i != o1.end (); ++i)
        delete *i;
    }

    #pragma db id
    ncomp id;

    #pragma db inverse(o2)
    std::vector<object1*> o1;
  };

  #pragma db view object(object2) object(object1)
  struct view
  {
    #pragma db column (object2::id.num3)
    unsigned short num;

    #pragma db column (object1::id.str3)
    std::string str;
  };
}

// Test object with composite id and version (optimistic concurrency).
//
#pragma db namespace table("t7_")
namespace test7
{
  #pragma db object optimistic
  struct object
  {
    object () {}
    object (const scomp& i, unsigned long n): id (i), num (n) {}

    #pragma db id
    scomp id;

    #pragma db version
    unsigned long ver;

    unsigned long num;
  };

  inline bool
  operator== (const object& x, const object& y)
  {
    return x.id == y.id && x.ver == y.ver && x.num == y.num;
  }
}

// Test composite NULL pointers.
//
#pragma db namespace table("t8_")
namespace test8
{
  #pragma db object
  struct object1
  {
    object1 () {}
    object1 (const scomp& i, unsigned long n): id (i), num (n) {}

    #pragma db id
    scomp id;

    unsigned long num;
  };

  inline bool
  operator== (const object1& x, const object1& y)
  {
    return x.id == y.id && x.num == y.num;
  }

  #pragma db object
  struct object2
  {
    object2 (): o1 (0) {}
    ~object2 () {delete o1;}

    #pragma db id auto
    unsigned long id;

    object1* o1;
  };

  #pragma db object
  struct object3
  {
    ~object3 ()
    {
      for (std::vector<object1*>::iterator i (o1.begin ());
           i != o1.end (); ++i)
        delete *i;
    }

    #pragma db id auto
    unsigned long id;

    std::vector<object1*> o1;
  };
}

// Test composite id definition inside object.
//
#pragma db namespace table("t9_")
namespace test9
{
  #pragma db object
  struct object
  {
    object (unsigned long n = 0, const std::string& s = "")
    {
      id_.num = n;
      id_.str = s;
    }

    unsigned long num () const {return id_.num;}
    const std::string& str () const {return id_.str;}

    std::vector<int> v;

  private:
    friend class odb::access;

    #pragma db value
    struct comp
    {
      unsigned long num;
      std::string str;

      bool
      operator< (const comp& x) const
      {
        return num < x.num || (num == x.num && str < x.str);
      }
    };

    #pragma db id
    comp id_;
  };

  inline bool
  operator== (const object& x, const object& y)
  {
    return x.num () == y.num () && x.str () == y.str () && x.v == y.v;
  }
}


#endif // TEST_HXX
