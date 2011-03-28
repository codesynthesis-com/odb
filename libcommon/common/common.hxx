// file      : libcommon/common/common.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_COMMON_HXX
#define LIBCOMMON_COMMON_COMMON_HXX

#include <memory>  // std::auto_ptr
#include <cstddef> // std::size_t

#include <odb/forward.hxx> // odb::database
#include <odb/result.hxx>

#include <common/config.hxx>
#include <common/export.hxx>

LIBCOMMON_EXPORT std::auto_ptr<odb::database>
create_database (int& argc,
                 char* argv[],
                 bool create_schema = true,
                 std::size_t max_connections = 0);

// This function returns an accurate result only if the result iterator
// hasn't been advanced.
//
template <typename T>
std::size_t
size (odb::result<T>& r)
{
  std::size_t n (0);
#if defined(DATABASE_SQLITE)
  for (typename odb::result<T>::iterator i (r.begin ()); i != r.end (); ++i)
    n++;
#else
  n = r.size ();
#endif
  return n;
}

#endif // LIBCOMMON_COMMON_COMMON_HXX
