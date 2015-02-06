// file      : libcommon/common/common.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_COMMON_HXX
#define LIBCOMMON_COMMON_COMMON_HXX

#include <memory>  // std::auto_ptr
#include <cstddef> // std::size_t

#include <odb/forward.hxx> // odb::database
#include <odb/result.hxx>

#include <common/export.hxx>

// Make sure assert() is not disabled.
//
#ifdef NDEBUG
#  error ODB tests require enabled assert(); un-define the NDEBUG macro
#endif

LIBCOMMON_EXPORT std::auto_ptr<odb::database>
create_database (int argc,
                 char* argv[],
                 bool create_schema = true,
                 std::size_t max_connections = 0,
                 odb::database_id db = odb::id_common);

template <typename T>
std::auto_ptr<T>
create_specific_database (int argc,
                          char* argv[],
                          bool create_schema = true,
                          std::size_t max_connections = 0)
{
  std::auto_ptr<odb::database> r (
    create_database (argc, argv,
                     create_schema,
                     max_connections,
                     T::database_id));

  return std::auto_ptr<T> (&dynamic_cast<T&> (*r.release ()));
}

// This function returns an accurate result only if the result iterator
// hasn't been advanced and after the call the result is no longer valid.
//
template <typename T>
std::size_t
size (odb::result<T>);

#include <common/common.txx>

#endif // LIBCOMMON_COMMON_COMMON_HXX
