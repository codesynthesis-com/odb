// file      : odb/mysql/forward.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_FORWARD_HXX
#define ODB_MYSQL_FORWARD_HXX

#include <odb/pre.hxx>

#include <odb/forward.hxx>

namespace odb
{
  namespace mysql
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
      using mysql::database;
      using mysql::connection;
      using mysql::connection_ptr;
      using mysql::transaction;
      using mysql::statement;
    }

    // Implementation details.
    //
    enum statement_kind
    {
      statement_select,
      statement_insert,
      statement_update,
      statement_delete
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
  }

  namespace details
  {
    template <>
    struct counter_type<mysql::connection>
    {
      typedef shared_base counter;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_FORWARD_HXX
