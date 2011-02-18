// file      : odb/query.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QUERY_HXX
#define ODB_QUERY_HXX

#include <odb/pre.hxx>

#include <odb/traits.hxx>

namespace odb
{
  template <typename T,
            typename Q = typename object_traits<T>::query_base_type>
  class query;

  namespace core
  {
    using odb::query;
  }
}

#include <odb/post.hxx>

#endif // ODB_QUERY_HXX
