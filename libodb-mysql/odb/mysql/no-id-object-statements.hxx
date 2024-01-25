// file      : odb/mysql/no-id-object-statements.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_NO_ID_OBJECT_STATEMENTS_HXX
#define ODB_MYSQL_NO_ID_OBJECT_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/details/shared-ptr.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/mysql.hxx>
#include <odb/mysql/binding.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/statements-base.hxx>

namespace odb
{
  namespace mysql
  {
    //
    // Implementation for objects without object id.
    //

    template <typename T>
    class no_id_object_statements: public statements_base
    {
    public:
      typedef T object_type;
      typedef object_traits_impl<object_type, id_mysql> object_traits;
      typedef typename object_traits::pointer_type pointer_type;
      typedef typename object_traits::image_type image_type;

      typedef mysql::insert_statement insert_statement_type;

    public:
      no_id_object_statements (connection_type&);

      virtual
      ~no_id_object_statements ();

      // Object image.
      //
      image_type&
      image () {return image_;}

      // Insert binding.
      //
      std::size_t
      insert_image_version () const { return insert_image_version_;}

      void
      insert_image_version (std::size_t v) {insert_image_version_ = v;}

      binding&
      insert_image_binding () {return insert_image_binding_;}

      // Select binding (needed for query support).
      //
      std::size_t
      select_image_version () const { return select_image_version_;}

      void
      select_image_version (std::size_t v) {select_image_version_ = v;}

      binding&
      select_image_binding () {return select_image_binding_;}

      my_bool*
      select_image_truncated () {return select_image_truncated_;}

      // Statements.
      //
      insert_statement_type&
      persist_statement ()
      {
        if (persist_ == 0)
          persist_.reset (
            new (details::shared) insert_statement_type (
              conn_,
              object_traits::persist_statement,
              object_traits::versioned, // Process if versioned.
              insert_image_binding_,
              0,
              false));

        return *persist_;
      }

    public:
      // select = total
      // insert = total - inverse; inverse == 0 for object without id
      //
      static const std::size_t select_column_count =
        object_traits::column_count;

      static const std::size_t insert_column_count =
        object_traits::column_count;

    private:
      no_id_object_statements (const no_id_object_statements&);
      no_id_object_statements& operator= (const no_id_object_statements&);

    private:
      image_type image_;

      // Select binding.
      //
      std::size_t select_image_version_;
      binding select_image_binding_;
      MYSQL_BIND select_image_bind_[select_column_count];
      my_bool select_image_truncated_[select_column_count];

      // Insert binding.
      //
      std::size_t insert_image_version_;
      binding insert_image_binding_;
      MYSQL_BIND insert_image_bind_[insert_column_count];

      details::shared_ptr<insert_statement_type> persist_;
    };
  }
}

#include <odb/mysql/no-id-object-statements.txx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_NO_ID_OBJECT_STATEMENTS_HXX
