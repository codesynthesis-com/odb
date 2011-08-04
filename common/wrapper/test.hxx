// file      : common/wrapper/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_TR1_MEMORY

#include <string>
#include <memory> // std::auto_ptr
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#ifdef HAVE_TR1_MEMORY
#  include <odb/tr1/memory.hxx>
#endif

using odb::nullable;

typedef nullable<std::string> nullable_string;

#ifdef HAVE_TR1_MEMORY
typedef std::tr1::shared_ptr<std::string> tr1_nullable_string;
#endif

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  std::auto_ptr<unsigned long> num;

  #pragma db null
  std::auto_ptr<std::string> str;

  nullable_string nstr;
  std::vector<nullable_string> nstrs;

#ifdef HAVE_TR1_MEMORY
  #pragma db null
  tr1_nullable_string tr1_str;

  #pragma db value_null
  std::vector<tr1_nullable_string> tr1_strs;
#endif
};

#endif // TEST_HXX
