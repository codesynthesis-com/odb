// file      : odb/oracle/view-statements.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_ORACLE_VIEW_STATEMENTS_HXX
#define ODB_ORACLE_VIEW_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/oracle/oracle-types.hxx>
#include <odb/oracle/version.hxx>
#include <odb/oracle/statement.hxx>
#include <odb/oracle/statements-base.hxx>

namespace odb
{
  namespace oracle
  {
    template <typename T>
    class view_statements: public statements_base
    {
    public:
      typedef T view_type;
      typedef odb::view_traits<view_type> view_traits;
      typedef typename view_traits::pointer_type pointer_type;
      typedef typename view_traits::image_type image_type;

    public:
      view_statements (connection_type&);

      virtual
      ~view_statements ();

      // View image.
      //
      image_type&
      image ()
      {
        return image_;
      }

      std::size_t
      image_version () const
      {
        return image_version_;
      }

      void
      image_version (std::size_t v)
      {
        image_version_ = v;
      }

      binding&
      image_binding ()
      {
        return image_binding_;
      }

      sb2*
      image_indicator ()
      {
        return image_indicator_;
      }

    private:
      view_statements (const view_statements&);
      view_statements& operator= (const view_statements&);

    private:
      image_type image_;
      std::size_t image_version_;
      binding image_binding_;
      bind image_bind_[view_traits::column_count];
      sb2 image_indicator_[view_traits::column_count];
    };
  }
}

#include <odb/oracle/view-statements.txx>

#include <odb/post.hxx>

#endif // ODB_ORACLE_VIEW_STATEMENTS_HXX
