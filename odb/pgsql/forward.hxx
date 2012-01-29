// file      : odb/pgsql/forward.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_FORWARD_HXX
#define ODB_PGSQL_FORWARD_HXX

#include <odb/pre.hxx>

#include <odb/forward.hxx>

namespace odb
{
  namespace pgsql
  {
    class database;
    class connection;
    typedef details::shared_ptr<connection> connection_ptr;
    class connection_factory;
    class statement;
    class transaction;
    class tracer;
    class query;

    // Implementation details.
    //
    enum statement_kind
    {
      statement_select,
      statement_insert,
      statement_update
    };

    class binding;
    class select_statement;

    template <typename T>
    class object_statements;

    template <typename T>
    class object_statements_no_id;

    template <typename T>
    class view_statements;

    template <typename T>
    class container_statements;
  }

  namespace details
  {
    template <>
    struct counter_type<pgsql::connection>
    {
      typedef shared_base counter;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_FORWARD_HXX
