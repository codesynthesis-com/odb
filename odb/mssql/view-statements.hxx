// file      : odb/mssql/view-statements.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_MSSQL_VIEW_STATEMENTS_HXX
#define ODB_MSSQL_VIEW_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/mssql/mssql-types.hxx>
#include <odb/mssql/version.hxx>
#include <odb/mssql/statement.hxx>
#include <odb/mssql/statements-base.hxx>

namespace odb
{
  namespace mssql
  {
    template <typename T>
    class view_statements: public statements_base
    {
    public:
      typedef T view_type;
      typedef view_traits_impl<view_type, id_mssql> view_traits;
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

    private:
      view_statements (const view_statements&);
      view_statements& operator= (const view_statements&);

    private:
      image_type image_;
      std::size_t image_version_;
      binding image_binding_;
      bind image_bind_[view_traits::column_count != 0
                       ? view_traits::column_count
                       : 1];
    };
  }
}

#include <odb/mssql/view-statements.txx>

#include <odb/post.hxx>

#endif // ODB_MSSQL_VIEW_STATEMENTS_HXX
