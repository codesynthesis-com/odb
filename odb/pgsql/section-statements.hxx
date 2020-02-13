// file      : odb/pgsql/section-statements.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_SECTION_STATEMENTS_HXX
#define ODB_PGSQL_SECTION_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/schema-version.hxx>
#include <odb/traits.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/binding.hxx>
#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/connection.hxx>
#include <odb/pgsql/database.hxx>
#include <odb/pgsql/pgsql-fwd.hxx> // Oid
#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
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

      typedef pgsql::select_statement select_statement_type;
      typedef pgsql::update_statement update_statement_type;

      typedef pgsql::connection connection_type;

      section_statements (connection_type&,
                          image_type&,
                          id_image_type&,
                          binding& id,
                          binding& idv,
                          native_binding& idn,
                          const Oid* idt);

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

      bool*
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
              traits::select_name,
              traits::select_statement,
              traits::versioned, // Process if versioned.
              false,             // Don't optimize.
              id_types_,
              id_column_count,
              id_binding_,
              id_native_binding_,
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
              traits::update_name,
              traits::update_statement,
              traits::versioned, // Process if versioned.
              traits::update_types,
              update_column_count + id_column_count +
              managed_optimistic_update_column_count,
              update_image_binding_,
              update_image_native_binding_,
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
      native_binding& id_native_binding_;
      const Oid* id_types_;

      // Select binding.
      //
      std::size_t select_image_version_;

      static const std::size_t select_bind_count =
        select_column_count != 0 || managed_optimistic_load_column_count != 0
        ? select_column_count + managed_optimistic_load_column_count
        : 1;

      binding select_image_binding_;
      bind select_image_bind_[select_bind_count];
      bool select_image_truncated_[select_bind_count];

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
      bind update_image_bind_[update_bind_count];

      native_binding update_image_native_binding_;
      char* update_image_values_[update_bind_count];
      int update_image_lengths_[update_bind_count];
      int update_image_formats_[update_bind_count];

      details::shared_ptr<select_statement_type> select_;
      details::shared_ptr<update_statement_type> update_;
    };
  }
}

#include <odb/pgsql/section-statements.txx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_SECTION_STATEMENTS_HXX
