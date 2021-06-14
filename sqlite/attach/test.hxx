// file      : sqlite/attach/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include <string>

#include <odb/core.hxx>

#pragma db object
struct object
{
  explicit
  object (const std::string& s): str (s) {}

  #pragma db id auto
  unsigned long id;

  #pragma db unique
  std::string str;

private:
  object () {}

  friend class odb::access;
};

#endif // TEST_HXX
