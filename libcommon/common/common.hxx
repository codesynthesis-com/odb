// file      : libcommon/common/common.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef LIBCOMMON_COMMON_COMMON_HXX
#define LIBCOMMON_COMMON_COMMON_HXX

#include <memory>  // std::auto_ptr
#include <cstddef> // std::size_t

#include <odb/database.hxx>

#include <common/export.hxx>

LIBCOMMON_EXPORT std::auto_ptr<odb::database>
create_database (int argc, char* argv[], std::size_t max_connections = 0);

#endif // LIBCOMMON_COMMON_COMMON_HXX
