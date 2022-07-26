// file      : pgsql/savepoint/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object bulk(100)
struct object
{
  object (unsigned long id)
      : id_ (id)
  {
  }

  object ()
  {
  }

  #pragma db id
  unsigned long id_;
};

#endif // TEST_HXX
