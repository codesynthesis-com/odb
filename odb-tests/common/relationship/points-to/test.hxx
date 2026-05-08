// file      : common/relationship/points-to/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <memory>  // std::shared_ptr
#include <vector>
#include <string>
#include <cstddef> // std::size_t
#include <utility> // std::move()

#include <odb/core.hxx>

// Note that besides tests for points_to-based object relationship we also add
// similar tests for pointer-based relationship, to make sure the behavior is
// consistent.
//

// Composite id.
//
#pragma db value
struct comp
{
  comp () {}
  comp (int n, std::string s): num (n), str (std::move (s)) {}

  int num = 0;
  std::string str;
};

inline bool
operator== (const comp& x, const comp& y)
{
  return x.num == y.num && x.str == y.str;
}

inline bool
operator!= (const comp& x, const comp& y)
{
  return !(x == y);
}

inline bool
operator< (const comp& x, const comp& y)
{
  return x.num != y.num ? x.num < y.num : x.str < y.str;
}

// Test column constraints generation for container members (regression for GH
// issue #9).
//
#pragma db namespace table("t1_") pointer(std::shared_ptr)
namespace test1
{
  // Simple id.
  //
  #pragma db namespace table("s_")
  namespace simple_id
  {
    #pragma db object
    struct obj
    {
      obj () {}
      obj (int i): id (i) {}

      #pragma db id
      int id;

      #pragma db points_to(obj)
      std::vector<int> ids;

      std::vector<std::shared_ptr<obj>> objs;
    };
  }

  // Composite id.
  //
  #pragma db namespace table("c_")
  namespace composite_id
  {
    #pragma db object
    struct obj
    {
      obj () {}
      obj (int n, std::string s): id (n, std::move (s)) {}

      #pragma db id
      comp id;

      #pragma db points_to(obj)
      std::vector<comp> ids;

      std::vector<std::shared_ptr<obj>> objs;
    };
  }

  // Return true if the objects match shallowly (for the referenced objects
  // only ids are being matched).
  //
  template <typename T>
  inline auto
  operator== (const T& x, const T& y)
    -> decltype (x.id == y.id && x.ids == y.ids && x.objs == y.objs)
  {
    if (x.id == y.id && x.ids == y.ids && x.objs.size () == y.objs.size ())
    {
      for (std::size_t i (0); i != x.objs.size (); ++i)
      {
        const std::shared_ptr<T>& xo (x.objs[i]);
        const std::shared_ptr<T>& yo (y.objs[i]);

        if ((xo == nullptr) != (yo == nullptr) ||
            (xo != nullptr && xo->id != yo->id))
        {
          return false;
        }
      }

      return true;
    }
    else
      return false;
  }
}

// Test association via points_to-declared members in object views (regression
// for GH issue #13).
//
#pragma db namespace table("t2_") pointer(std::shared_ptr)
namespace test2
{
  // Simple id.
  //
  #pragma db namespace table("s_")
  namespace simple_id
  {
    #pragma db object
    struct obj1
    {
      obj1 () {}
      obj1 (int i, int f): id (i), field1 (f) {}

      #pragma db id
      int id;

      int field1;
    };

    #pragma db object
    struct obj2
    {
      obj2 () {}
      obj2 (int i, int f, int r): id (i), field2 (f), ref (r) {}

      #pragma db id
      int id;

      int field2;

      #pragma db points_to(obj1)
      int ref;
    };

    #pragma db object
    struct obj3
    {
      obj3 () {}

      obj3 (int i, int f, std::shared_ptr<obj1> r)
          : id (i), field3 (f), ref (std::move (r)) {}

      #pragma db id
      int id;

      int field3;

      std::shared_ptr<obj1> ref;
    };

    #pragma db view object(obj1) object(obj2)
    struct view1
    {
      int field1;
      int field2;
    };

    #pragma db view object(obj1) object(obj3)
    struct view2
    {
      int field1;
      int field3;
    };
  }

  // Composite id.
  //
  #pragma db namespace table("c_")
  namespace composite_id
  {
    #pragma db object
    struct obj1
    {
      obj1 () {}
      obj1 (int n, std::string s, int f): id (n, std::move (s)), field1 (f) {}

      #pragma db id
      comp id;

      int field1;
    };

    #pragma db object
    struct obj2
    {
      obj2 () {}
      obj2 (int i, int f, comp r): id (i), field2 (f), ref (std::move (r)) {}

      #pragma db id
      int id;

      int field2;

      #pragma db points_to(obj1)
      comp ref;
    };

    #pragma db object
    struct obj3
    {
      obj3 () {}

      obj3 (int i, int f, std::shared_ptr<obj1> r)
          : id (i), field3 (f), ref (std::move (r)) {}

      #pragma db id
      int id;

      int field3;

      std::shared_ptr<obj1> ref;
    };

    #pragma db view object(obj1) object(obj2)
    struct view1
    {
      int field1;
      int field2;
    };

    #pragma db view object(obj1) object(obj3)
    struct view2
    {
      int field1;
      int field3;
    };
  }
}

#endif // TEST_HXX
