// file      : mysql/truncation/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>
#include <vector>

#include <odb/core.hxx>

#pragma db object table ("test")
struct object1
{
  object1 () {}
  object1 (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  std::string str_;
};

#pragma db object table ("test")
struct object2
{
  object2 () {}
  object2 (unsigned long id): id_ (id) {}

  #pragma db id
  unsigned long id_;

  std::string str_;
};

#pragma db object
struct container
{
  container () {}
  container (unsigned long id) : id_ (id) {}

  #pragma db id
  unsigned long id_;

  std::vector<std::string> vec_;
};

#endif // TEST_HXX
