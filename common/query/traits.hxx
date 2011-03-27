// file      : common/query/traits.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TRAITS_HXX
#define TRAITS_HXX

#include <common/config.hxx>

#if defined(DATABASE_MYSQL)
#  include "traits-mysql.hxx"
#elif defined(DATABASE_SQLITE)
#  include "traits-sqlite.hxx"
#endif

#endif // TRAITS_HXX
