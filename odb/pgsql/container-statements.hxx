// file      : odb/pgsql/container-statements.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_CONTAINER_STATEMENTS_HXX
#define ODB_PGSQL_CONTAINER_STATEMENTS_HXX

#include <odb/pre.hxx>

#include <cstddef> // std::size_t

#include <odb/forward.hxx>
#include <odb/traits.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/binding.hxx>
#include <odb/pgsql/statement.hxx>
#include <odb/pgsql/pgsql-oid.hxx>
#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class connection;

    // Template argument is the generated abstract container traits type.
    // That is, it doesn't need to provide column counts and statements.
    //
    template <typename T>
    class container_statements
    {
    public:
      typedef T traits;

      typedef typename traits::data_image_type data_image_type;
      typedef typename traits::cond_image_type cond_image_type;

      typedef typename traits::functions_type functions_type;

      typedef pgsql::insert_statement insert_statement_type;
      typedef pgsql::select_statement select_statement_type;
      typedef pgsql::delete_statement delete_statement_type;

      typedef pgsql::connection connection_type;

      container_statements (connection_type&);

      connection_type&
      connection ()
      {
        return conn_;
      }

      // Functions.
      //
      functions_type&
      functions ()
      {
        return functions_;
      }

      // Id image binding (external).
      //
      const binding&
      id_binding ()
      {
        return *id_binding_;
      }

      void
      id_binding (const binding& b)
      {
        id_binding_ = &b;
      }

      // Condition image.
      //
      cond_image_type&
      cond_image ()
      {
        return cond_image_;
      }

      std::size_t
      cond_image_version () const
      {
        return cond_image_version_;
      }

      void
      cond_image_version (std::size_t v)
      {
        cond_image_version_ = v;
      }

      std::size_t
      cond_id_binding_version () const
      {
        return cond_id_binding_version_;
      }

      void
      cond_id_binding_version (std::size_t v)
      {
        cond_id_binding_version_ = v;
      }

      binding&
      cond_image_binding ()
      {
        return cond_image_binding_;
      }

      native_binding&
      cond_image_native_binding ()
      {
        return cond_image_native_binding_;
      }

      // Data image.
      //
      data_image_type&
      data_image ()
      {
        return data_image_;
      }

            std::size_t
      data_image_version () const
      {
        return data_image_version_;
      }

      void
      data_image_version (std::size_t v)
      {
        data_image_version_ = v;
      }

      std::size_t
      data_id_binding_version () const
      {
        return data_id_binding_version_;
      }

      void
      data_id_binding_version (std::size_t v)
      {
        data_id_binding_version_ = v;
      }

      binding&
      data_image_binding ()
      {
        return data_image_binding_;
      }

      bool*
      data_image_truncated ()
      {
        return data_image_truncated_;
      }

      native_binding&
      data_image_native_binding ()
      {
        return data_image_native_binding_;
      }

      //
      // Statements.
      //

      insert_statement_type&
      insert_one_statement ()
      {
        if (insert_one_ == 0)
        {
          insert_one_.reset (
            new (details::shared) insert_statement_type (
              conn_,
              insert_one_name_,
              insert_one_text_,
              insert_one_types_,
              insert_one_count_,
              data_image_binding_,
              data_image_native_binding_));
        }

        return *insert_one_;
      }

      select_statement_type&
      select_all_statement ()
      {
        if (select_all_ == 0)
        {
          select_all_.reset (
            new (details::shared) select_statement_type (
              conn_,
              select_all_name_,
              select_all_text_,
              select_all_types_,
              select_all_count_,
              cond_image_binding_,
              cond_image_native_binding_,
              data_image_binding_));
        }

        return *select_all_;
      }

      delete_statement_type&
      delete_all_statement ()
      {
        if (delete_all_ == 0)
        {
          delete_all_.reset (
            new (details::shared) delete_statement_type (
              conn_,
              delete_all_name_,
              delete_all_text_,
              delete_all_types_,
              delete_all_count_,
              cond_image_binding_,
              cond_image_native_binding_));
        }

        return *delete_all_;
      }

    private:
      container_statements (const container_statements&);
      container_statements& operator= (const container_statements&);

    protected:
      connection_type& conn_;
      functions_type functions_;

      const binding* id_binding_;

      cond_image_type cond_image_;
      std::size_t cond_image_version_;
      std::size_t cond_id_binding_version_;
      binding cond_image_binding_;
      bind* cond_image_bind_;
      native_binding cond_image_native_binding_;

      data_image_type data_image_;
      std::size_t data_image_version_;
      std::size_t data_id_binding_version_;
      binding data_image_binding_;
      bind* data_image_bind_;
      bool* data_image_truncated_;
      native_binding data_image_native_binding_;

      const char* insert_one_name_;
      const char* insert_one_text_;
      const Oid* insert_one_types_;
      std::size_t insert_one_count_;

      const char* select_all_name_;
      const char* select_all_text_;
      const Oid* select_all_types_;
      std::size_t select_all_count_;

      const char* delete_all_name_;
      const char* delete_all_text_;
      const Oid* delete_all_types_;
      std::size_t delete_all_count_;

      details::shared_ptr<insert_statement_type> insert_one_;
      details::shared_ptr<select_statement_type> select_all_;
      details::shared_ptr<delete_statement_type> delete_all_;
    };

    // Template argument is the generated concrete container traits type.
    //
    template <typename T>
    class container_statements_impl: public T::statements_type
    {
    public:
      typedef T traits;
      typedef typename T::statements_type base;
      typedef pgsql::connection connection_type;

      container_statements_impl (connection_type&);

    private:
      container_statements_impl (const container_statements_impl&);
      container_statements_impl& operator= (const container_statements_impl&);

    private:
      bind cond_image_bind_array_[traits::cond_column_count];
      const char* cond_image_values_[traits::cond_column_count];
      int cond_image_lengths_[traits::cond_column_count];
      int cond_image_formats_[traits::cond_column_count];

      bind data_image_bind_array_[traits::data_column_count];
      const char* data_image_values_[traits::data_column_count];
      int data_image_lengths_[traits::data_column_count];
      int data_image_formats_[traits::data_column_count];

      bool data_image_truncated_array_[traits::data_column_count];
    };
  }
}

#include <odb/pgsql/container-statements.txx>

#include <odb/post.hxx>

#endif // ODB_PGSQL_CONTAINER_STATEMENTS_HXX
