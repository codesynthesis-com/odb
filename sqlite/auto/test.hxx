// file      : sqlite/auto/test.hxx
// copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
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
  object (const std::string& str): str_ (str) {}
  object (unsigned long id, const std::string& str): id_ (id), str_ (str) {}

  #pragma db auto id
  odb::nullable<unsigned long> id_;

  std::string str_;

private:
  object () {}

  friend class odb::access;
};

#endif // TEST_HXX
