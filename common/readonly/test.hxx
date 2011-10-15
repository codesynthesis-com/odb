// file      : common/readonly/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>

#include <odb/core.hxx>

// Simple readonly value.
//
#pragma db object
struct simple
{
  simple (unsigned long i, unsigned long x)
      : id (i), ro (x), /*co (x),*/ rw (x)
  {
  }

  simple ()/*: co (0)*/ {}

  #pragma db id
  unsigned long id;

  #pragma db readonly
  unsigned long ro;

  //const unsigned long co;

  unsigned long rw;
};

// Readonly pointer.
//
#pragma db object
struct pointer
{
  pointer (unsigned long i, pointer* p = 0): id (i), ro (p), rw (p) {}
  pointer (): ro (0), rw (0) {}

  ~pointer ()
  {
    delete ro;

    if (ro != rw)
      delete rw;
  }

  #pragma db id
  unsigned long id;

  #pragma db readonly
  pointer* ro;

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
  value () {}
  value (unsigned long x): ro_value (x), ro (x), rw (x) {}

  #pragma db readonly
  unsigned long ro;

  unsigned long rw;
};

#pragma db object
struct composite
{
  composite (unsigned long i, unsigned long x)
      : id (i), ro (x), rw (x), v (x)
  {
  }

  composite () {}

  #pragma db id
  unsigned long id;

  #pragma db readonly
  value ro;

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

  std::vector<unsigned long> rw;
};

// Readonly object.
//
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

#endif // TEST_HXX
