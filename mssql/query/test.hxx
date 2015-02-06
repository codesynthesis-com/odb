// file      : mssql/query/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <odb/core.hxx>

#pragma db object
struct object
{
  #pragma db id auto
  unsigned long id_;

  unsigned int num;

  #pragma db type ("SMALLMONEY")
  int smoney;

  #pragma db type ("MONEY")
  double money;

  #pragma db type ("VARCHAR(256)")
  std::string str;

  #pragma db type ("NVARCHAR(128)")
  std::wstring nstr;

  #pragma db type ("VARCHAR(max)")
  std::string lstr;

  #pragma db type ("NVARCHAR(max)")
  std::wstring lnstr;
};

#endif // TEST_HXX
