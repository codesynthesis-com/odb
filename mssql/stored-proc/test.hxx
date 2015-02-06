// file      : mssql/stored-proc/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db view query("SELECT SCHEMA_NAME()")
struct default_schema
{
  std::string name;
};

#pragma db object
struct object
{
  object () {}
  object (unsigned int n, std::string s): num (n), str (s) {}

  #pragma db id auto
  unsigned long id;

  unsigned int num;
  std::string str;
};

#pragma db view
struct no_result {};

#pragma db view query("EXEC select_all_objects")
struct select_all_objects
{
  unsigned int num;
  std::string str;
};

#pragma db view query("EXEC select_objects (?)")
struct select_objects
{
  std::string str;
};

#pragma db view query("EXEC objects_min_max_odb")
struct objects_min_max
{
  unsigned int num_min;
  unsigned int num_max;
};

#pragma db view query("EXEC insert_object (?)")
struct insert_object {};

#pragma db view query("EXEC insert_object_id_odb (?)")
struct insert_object_id
{
  unsigned int ret;
  unsigned long id;
};

#endif // TEST_HXX
