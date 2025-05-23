// file      : common/definition/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#ifdef _WIN32
#  include <winsock2.h> // timeval
#else
#  include <sys/time.h> // timeval
#endif

#include <odb/core.hxx>

#include "time-mapping.hxx"

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id;

  timeval time;
};

#endif // TEST_HXX
