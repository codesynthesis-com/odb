// file      : common/readonly/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>
#include <memory> // std::auto_ptr

#include <odb/core.hxx>

// Simple readonly object.
//
#pragma db object
struct simple
{
  simple (unsigned long i, unsigned long x)
      : id (i), ro (x), co (x), rw (x)
  {
  }

  simple (): co (0) {}

  #pragma db id
  unsigned long id;

  #pragma db readonly
  unsigned long ro;

  const unsigned long co;

  unsigned long rw;
};

// Readonly pointer.
//
#pragma db object
struct pointer
{
  pointer (unsigned long i, pointer* p = 0): id (i), ro (p), co (p), rw (p) {}
  pointer (): ro (0), co (0), rw (0) {}

  ~pointer ()
  {
    delete ro;

    if (co != ro)
      delete co;

    if (rw != ro && rw != co)
      delete rw;
  }

  #pragma db id
  unsigned long id;

  #pragma db readonly
  pointer* ro;

  pointer* const co;

  pointer* rw;
};

// Composite readonly value as well as simple readonly value inside
// a composite.
//
#pragma db value readonly
struct ro_value
{
  ro_value () {}
  ro_value (unsigned long x): v (x) {}

  unsigned long v;
};

#pragma db value
struct value: ro_value
{
  value (): co (0) {}
  value (unsigned long x): ro_value (x), ro (x), co (x), rw (x) {}

  #pragma db readonly
  unsigned long ro;

  const unsigned long co;

  unsigned long rw;
};

#pragma db object
struct composite
{
  composite (unsigned long i, unsigned long x)
      : id (i), ro (x), co (x), rw (x), v (x)
  {
  }

  composite () {}

  #pragma db id
  unsigned long id;

  #pragma db readonly
  value ro;

  const value co;

  value rw;
  ro_value v;
};

// Readonly container.
//
#pragma db object
struct container
{
  container (unsigned long i): id (i) {}
  container () {}

  #pragma db id
  unsigned long id;

  #pragma db readonly
  std::vector<unsigned long> ro;

  const std::vector<unsigned long> co;

  std::vector<unsigned long> rw;
};

// Readonly object.
//
#pragma db object readonly
struct simple_object
{
  simple_object (unsigned long i, unsigned long x): id (i), sv (x) {}
  simple_object () {}

  #pragma db id
  unsigned long id;

  unsigned long sv;
};

#pragma db object
struct object
{
  object (unsigned long i, unsigned long x): id (i), sv (x) {}
  object () {}

  #pragma db id
  unsigned long id;

  unsigned long sv;
};

#pragma db object readonly
struct ro_object: object
{
  ro_object (unsigned long i, unsigned long x)
      : object (i, x), cv (x)
  {
  }

  ro_object () {}

  value cv;
  std::vector<unsigned long> cr;
};

#pragma db object
struct rw_object: ro_object
{
  rw_object (unsigned long i, unsigned long x)
      : ro_object (i, x), rw_sv (x)
  {
  }

  rw_object () {}

  unsigned long rw_sv;
};

// Readonly wrappers. Here we make sure that only const wrappers with
// const wrapped types are automatically treated as readonly.
//
#pragma db object
struct wrapper
{
  wrapper (unsigned long i, unsigned long x)
      : id (i),
        pl (new unsigned long (x)),
        cpl (new unsigned long (x)),
        pcl (new unsigned long (x)),
        cpcl (new unsigned long (x))
  {
  }

  wrapper () {}

  #pragma db id
  unsigned long id;

  std::auto_ptr<unsigned long> pl;
  const std::auto_ptr<unsigned long> cpl;
  std::auto_ptr<const unsigned long> pcl;
  const std::auto_ptr<const unsigned long> cpcl;
};

// Readonly object with auto id.
//
#pragma db object readonly
struct ro_auto
{
  ro_auto (unsigned long n): num (n) {}
  ro_auto () {}

  #pragma db id auto
  unsigned long id;

  unsigned long num;
};

#endif // TEST_HXX
