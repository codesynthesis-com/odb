// file      : common/session/cache/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <memory>
#include <vector>

#include <odb/core.hxx>

// Test the session_required exception.
//
#pragma db namespace table("t1_")
namespace test1
{
  using std::shared_ptr;
  using std::weak_ptr;

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

#endif // TEST_HXX
