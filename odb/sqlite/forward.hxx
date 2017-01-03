// file      : odb/sqlite/forward.hxx
// copyright : Copyright (c) 2005-2017 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_SQLITE_FORWARD_HXX
#define ODB_SQLITE_FORWARD_HXX

#include <odb/pre.hxx>

#include <odb/forward.hxx>

namespace odb
{
  namespace sqlite
  {
    namespace core
    {
      using namespace odb::common;
    }

    //
    //
    class database;
    class connection;
    typedef details::shared_ptr<connection> connection_ptr;
    class connection_factory;
    class statement;
    class transaction;
    class tracer;

    namespace core
    {
      using sqlite::database;
      using sqlite::connection;
      using sqlite::connection_ptr;
      using sqlite::transaction;
      using sqlite::statement;
    }

    // Implementation details.
    //
    enum database_type_id
    {
      id_integer,
      id_real,
      id_text,
      id_blob,
      id_text_stream,
      id_blob_stream
    };

    template <typename T, database_type_id>
    struct default_value_traits;

    enum statement_kind
    {
      statement_select,
      statement_insert,
      statement_update,
      statement_delete,
      statement_generic
    };

    class binding;
    class select_statement;

    template <typename T>
    class object_statements;

    template <typename T>
    class polymorphic_root_object_statements;

    template <typename T>
    class polymorphic_derived_object_statements;

    template <typename T>
    class no_id_object_statements;

    template <typename T>
    class view_statements;

    template <typename T>
    class container_statements;

    template <typename T>
    class smart_container_statements;

    template <typename T, typename ST>
    class section_statements;

    class query_base;
    class query_params;
  }

  namespace details
  {
    template <>
    struct counter_type<sqlite::connection>
    {
      typedef shared_base counter;
    };

    template <>
    struct counter_type<sqlite::query_params>
    {
      typedef shared_base counter;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_SQLITE_FORWARD_HXX
