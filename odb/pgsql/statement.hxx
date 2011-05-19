// file      : odb/pgsql/statement.hxx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_STATEMENT_HXX
#define ODB_PGSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <libpq-fe.h>

#include <odb/details/shared-ptr.hxx>

#include <odb/pgsql/version.hxx>
#include <odb/pgsql/binding.hxx>

#include <odb/pgsql/details/export.hxx>

namespace odb
{
  namespace pgsql
  {
    class connection;

    class LIBODB_PGSQL_EXPORT statement: public details::shared_base
    {
    public:
      virtual
      ~statement () = 0;

    protected:
      statement (connection&,
                 const std::string& name,
                 const std::string& stmt,
                 const Oid* types,
                 std::size_t n);

    protected:
      connection& conn_;
      std::string name_;

    private:
      void
      release ();
    };

    class LIBODB_PGSQL_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection& conn,
                        const std::string& name,
                        const std::string& stmt,
                        const Oid* types,
                        std::size_t n,
                        native_binding& data);

      // Return true if successful and false if the row is a duplicate.
      // All other errors are reported by throwing exceptions.
      //
      bool
      execute ();

      unsigned long long
      id ();

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

    private:
      native_binding& data_;
      Oid oid_;
    };

    class LIBODB_PGSQL_EXPORT delete_statement: public statement
    {
    public:
      virtual
      ~delete_statement ();

      delete_statement (connection& conn,
                        const std::string& name,
                        const std::string& stmt,
                        const Oid* types,
                        std::size_t n,
                        native_binding& cond);

      unsigned long long
      execute ();

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);

    private:
      native_binding& cond_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_STATEMENT_HXX
