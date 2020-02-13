// file      : common/const-object/test.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <memory>
#include <odb/core.hxx>

#pragma db object pointer (obj1*) session
struct obj1
{
  obj1 () {}
  obj1 (int i): id (i) {}

  #pragma db id
  int id;

  void f () {}
  void cf () const {}
};

#ifdef HAVE_CXX11
#pragma db object pointer (std::unique_ptr<obj2>)
#else
#pragma db object pointer (std::auto_ptr<obj2>)
#endif
struct obj2
{
  obj2 () {}
  obj2 (int i): id (i) {}

  #pragma db id
  int id;

  void f () {}
  void cf () const {}
};

#pragma db object
struct aggr
{
  aggr (int i): id (i), o1 (0) {}
  aggr (): o1 (0) {}
  ~aggr () {delete o1;}

  #pragma db id
  int id;

  const obj1* o1;

#ifdef HAVE_CXX11
  std::unique_ptr<const obj2> o2;
#else
  std::auto_ptr<const obj2> o2;
#endif
};

#endif // TEST_HXX
