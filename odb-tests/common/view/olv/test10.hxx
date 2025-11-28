// file      : common/view/olv/test10.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST10_HXX
#define TEST10_HXX

#include <string>
#include <memory>  // weak/shared_ptr
#include <utility> // std::move

#include <odb/core.hxx>

// Test loading objects with weak pointers.
//
#pragma db namespace table("t10_") pointer(std::shared_ptr) session
namespace test10
{
  using std::shared_ptr;
  using std::weak_ptr;

  struct object2;

  #pragma db object
  struct object1
  {
    explicit
    object1 (int n_ = 0): n (n_) {}

    #pragma db id
    int n;

    shared_ptr<object2> o2;
  };

  #pragma db object no_id
  struct object2
  {
    explicit
    object2 (std::string s_ = ""): s (s_) {}

    #pragma db id
    std::string s;

    #pragma db inverse(o2)
    weak_ptr<object1> o1;
  };

  #pragma db view object(object1) object(object2)
  struct view1
  {
    shared_ptr<object1> o1;
    weak_ptr<object2> o2;
  };
}

#endif // TEST10_HXX
