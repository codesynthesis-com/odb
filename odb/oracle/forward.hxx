// file      : odb/oracle/forward.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

#ifndef ODB_ORACLE_FORWARD_HXX
#define ODB_ORACLE_FORWARD_HXX

#include <odb/pre.hxx>

#include <odb/forward.hxx>

namespace odb
{
  namespace oracle
  {
    class database;
    class connection;
    typedef details::shared_ptr<connection> connection_ptr;
    class connection_factory;
    class transaction;
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
    struct counter_type<oracle::connection>
    {
      typedef shared_base counter;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_ORACLE_FORWARD_HXX
