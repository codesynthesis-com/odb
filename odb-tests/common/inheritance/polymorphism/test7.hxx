// file      : common/inheritance/polymorphism/test7.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST7_HXX
#define TEST7_HXX

#include <string>
#include <memory>

#include <odb/core.hxx>

// Test polymorphism and object cache (session).
//
#pragma db namespace table("t7_")
namespace test7
{
  using std::shared_ptr;

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

#endif // TEST7_HXX
