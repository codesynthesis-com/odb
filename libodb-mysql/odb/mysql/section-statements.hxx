// file      : odb/mysql/section-statements.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_SECTION_STATEMENTS_HXX
#define ODB_MYSQL_SECTION_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/schema-version.hxx>
#include <odb/traits.hxx>

#include <odb/mysql/version.hxx>
#include <odb/mysql/mysql.hxx>
#include <odb/mysql/binding.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/database.hxx>
#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class connection;

    // Template argument is the section traits type.
    //
    template <typename T, typename ST>
    class section_statements
    {
    public:
      typedef ST traits;

      typedef typename traits::image_type image_type;
      typedef typename traits::id_image_type id_image_type;

      typedef mysql::select_statement select_statement_type;
      typedef mysql::update_statement update_statement_type;

      typedef mysql::connection connection_type;

      section_statements (connection_type&,
                          image_type&, id_image_type&,
                          binding& id, binding& idv);

      connection_type&
      connection () {return conn_;}

      const schema_version_migration&
      version_migration (const char* name = "") const
      {
        if (svm_ == 0)
          svm_ = &conn_.database ().schema_version_migration (name);

        return *svm_;
      }

      image_type&
      image () {return image_;}

      const binding&
      id_binding () {return id_binding_;}

      // Id and optimistic concurrency version (if any).
      //
      const binding&
      idv_binding () {return idv_binding_;}

      // Select binding.
      //
      std::size_t
      select_image_version () const { return select_image_version_;}

      void
      select_image_version (std::size_t v) {select_image_version_ = v;}

      binding&
      select_image_binding () {return select_image_binding_;}

      my_bool*
      select_image_truncated () {return select_image_truncated_;}

      // Update binding.
      //
      std::size_t
      update_image_version () const { return update_image_version_;}

      void
      update_image_version (std::size_t v) {update_image_version_ = v;}

      std::size_t
      update_id_binding_version () const { return update_id_binding_version_;}

      void
      update_id_binding_version (std::size_t v) {update_id_binding_version_ = v;}

      binding&
      update_image_binding () {return update_image_binding_;}

      //
      // Statements.
      //

      select_statement_type&
      select_statement ()
      {
        if (select_ == 0)
          select_.reset (
            new (details::shared) select_statement_type (
              conn_,
              traits::select_statement,
              traits::versioned, // Process if versioned.
              false,             // Don't optimize.
              id_binding_,
              select_image_binding_,
              false));

        return *select_;
      }

      update_statement_type&
      update_statement ()
      {
        if (update_ == 0)
          update_.reset (
            new (details::shared) update_statement_type (
              conn_,
              traits::update_statement,
              traits::versioned, // Process if versioned.
              update_image_binding_,
              false));

        return *update_;
      }

    public:
      static const std::size_t id_column_count = traits::id_column_count;
      static const std::size_t managed_optimistic_load_column_count =
        traits::managed_optimistic_load_column_count;
      static const std::size_t managed_optimistic_update_column_count =
        traits::managed_optimistic_update_column_count;
      static const std::size_t select_column_count = traits::load_column_count;
      static const std::size_t update_column_count =
        traits::update_column_count;

    private:
      section_statements (const section_statements&);
      section_statements& operator= (const section_statements&);

    protected:
      connection_type& conn_;
      mutable const schema_version_migration* svm_;

      // These come from object_statements.
      //
      image_type& image_;
      binding& id_binding_;
      binding& idv_binding_;

      // Select binding.
      //
      std::size_t select_image_version_;

      static const std::size_t select_bind_count =
        select_column_count != 0 || managed_optimistic_load_column_count != 0
        ? select_column_count + managed_optimistic_load_column_count
        : 1;

      binding select_image_binding_;
      MYSQL_BIND select_image_bind_[select_bind_count];
      my_bool select_image_truncated_[select_bind_count];

      // Update binding.
      //
      std::size_t update_image_version_;
      std::size_t update_id_binding_version_;

      static const std::size_t update_bind_count =
        update_column_count != 0 || managed_optimistic_update_column_count != 0
        ? update_column_count + id_column_count +
          managed_optimistic_update_column_count
        : 1;

      binding update_image_binding_;
      MYSQL_BIND update_image_bind_[update_bind_count];

      details::shared_ptr<select_statement_type> select_;
      details::shared_ptr<update_statement_type> update_;
    };
  }
}

#include <odb/mysql/section-statements.txx>

#include <odb/post.hxx>

#endif // ODB_MYSQL_SECTION_STATEMENTS_HXX
