// file      : boost/common/optional/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <boost/optional.hpp>

#include <odb/core.hxx>

#pragma db object
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

  boost::optional<std::string> str;
};

#endif // TEST_HXX
