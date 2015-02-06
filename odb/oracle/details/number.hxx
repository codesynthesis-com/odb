// file      : odb/oracle/details/number.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_NUMBER_HXX
#define ODB_ORACLE_NUMBER_HXX

#include <odb/pre.hxx>

#include <cstddef>  // std::size_t

#include <odb/oracle/details/export.hxx>

namespace odb
{
  // @@ Revise this.
  //
  namespace details
  {
  }

  namespace oracle
  {
    namespace details
    {
      using namespace odb::details;

      LIBODB_ORACLE_EXPORT long long
      number_to_int64 (const char* buffer, std::size_t n);

      LIBODB_ORACLE_EXPORT void
      int64_to_number (char* buffer, std::size_t& n, long long val);

      LIBODB_ORACLE_EXPORT unsigned long long
      number_to_uint64 (const char* buffer, std::size_t n);

      LIBODB_ORACLE_EXPORT void
      uint64_to_number (char* buffer, std::size_t& n, unsigned long long val);
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_NUMBER_HXX
