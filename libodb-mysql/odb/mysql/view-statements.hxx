// file      : odb/mysql/view-statements.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_VIEW_STATEMENTS_HXX
#define ODB_MYSQL_VIEW_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/version.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/statements-base.hxx>

namespace odb
{
  namespace mysql
  {
    template <typename T>
    class view_statements: public statements_base
    {
    public:
      typedef T view_type;
      typedef view_traits_impl<view_type, id_mysql> view_traits;
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

      my_bool*
      image_truncated ()
      {
        return image_truncated_;
      }

    private:
      view_statements (const view_statements&);
      view_statements& operator= (const view_statements&);

    private:
      image_type image_;
      std::size_t image_version_;
      binding image_binding_;
      MYSQL_BIND image_bind_[view_traits::column_count];
      my_bool image_truncated_[view_traits::column_count];
    };
  }
}

#include <odb/mysql/view-statements.txx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_VIEW_STATEMENTS_HXX
