// file      : odb/pgsql/view-result.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_VIEW_RESULT_HXX
#define ODB_PGSQL_VIEW_RESULT_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/forward.hxx> // query, view_statements
#include <odb/pgsql/result.hxx>
#include <odb/pgsql/statement.hxx>

namespace odb
{
  namespace pgsql
  {
    template <typename T>
    class result_impl<T, class_view>:
      public odb::result_impl<T, class_view>
    {
    public:
      typedef odb::result_impl<T, class_view> base_type;

      typedef typename base_type::pointer_type pointer_type;
      typedef typename base_type::pointer_traits pointer_traits;

      typedef typename base_type::view_type view_type;
      typedef typename base_type::view_traits view_traits;


      virtual
      ~result_impl ();

      result_impl (const query&,
                   details::shared_ptr<select_statement>,
                   view_statements<view_type>&);

      virtual void
      load (view_type&);

      virtual void
      next ();

      virtual void
      cache ();

      virtual std::size_t
      size ();

      using base_type::current;

    private:
      details::shared_ptr<select_statement> statement_;
      view_statements<view_type>& statements_;
    };
  }
}

#include <odb/pgsql/view-result.txx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_VIEW_RESULT_HXX
