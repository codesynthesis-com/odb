// file      : common/inheritance/polymorphism/test15.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST15_HXX
#define TEST15_HXX

#include <vector>

#include <odb/core.hxx>

#ifdef ODB_COMPILER
#  if defined(ODB_DATABASE_PGSQL)
#    define BLOB_TYPE "BYTEA"
#  elif defined(ODB_DATABASE_MSSQL)
#    define BLOB_TYPE "VARBINARY(max)"
#  else
#    define BLOB_TYPE "BLOB"
#  endif
#endif


// Test LOB/long data and polymorphism.
//
#pragma db namespace table("t15_")
namespace test15
{
  #pragma db object polymorphic
  struct base
  {
    virtual ~base () {}

    #pragma db id auto
    unsigned long id;
  };

  #pragma db object
  struct derived: base
  {
    #pragma db type(BLOB_TYPE)
    std::vector<char> blob;
  };
}

#endif // TEST15_HXX
