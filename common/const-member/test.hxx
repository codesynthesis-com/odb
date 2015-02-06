// file      : common/const-member/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <vector>
#include <string>
#include <memory> // std::auto_ptr

#include <odb/core.hxx>

// Const ids.
//
#pragma db object
struct const_id
{
  const_id (unsigned long i): id (i) {}
  const_id (): id (0) {}

  #pragma db id
  const unsigned long id;
};

#pragma db object
struct const_auto_id
{
  const_auto_id (): id (0) {}

  #pragma db id auto
  const unsigned long id;
};

// Container.
//
#pragma db value
struct container_value
{
  container_value (unsigned long x)
  {
    vec.push_back (x);
    const_cast<std::vector<unsigned long>&> (cvec).push_back (x);
  }

  container_value () {}

  std::vector<unsigned long> vec;
  const std::vector<unsigned long> cvec;
};

#pragma db object
struct container
{
  container (unsigned long i, unsigned long x)
      : id (i), ccom (x)
  {
    const_cast<std::vector<unsigned long>&> (cvec).push_back (x);
  }

  container () {}

  #pragma db id
  unsigned long id;

  const container_value ccom;
  const std::vector<unsigned long> cvec;
};

// Wrapper.
//
#pragma db value
struct wrapped_value
{
  wrapped_value (const std::string& s, unsigned long n)
      : str (s), num (n)
  {
    vec.push_back (n);
  }

  wrapped_value () {}

  const std::string str;
  unsigned long num;
  std::vector<unsigned long> vec;
};

#pragma db object
struct wrapper
{
  wrapper (unsigned long i, const std::string& s, unsigned long n)
      : id (i),
        str (new std::string (s)),
        com (new wrapped_value (s, n)),
        vec (new std::vector<unsigned long>)
  {
    const_cast<std::vector<unsigned long>&> (*vec).push_back (n);
  }

  wrapper () {}

  #pragma db id
  unsigned long id;

  const std::auto_ptr<const std::string> str;
  const std::auto_ptr<const wrapped_value> com;
  const std::auto_ptr< const std::vector<unsigned long> > vec;
};

#endif // TEST_HXX
