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
#include <odb/pgsql/result-ptr.hxx>

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

      void
      deallocate ();

      // Adapt an ODB binding to a native PostgreSQL parameter binding.
      //
      static void
      bind_param (native_binding&, const binding&);

      // Populate an ODB binding given a PostgreSQL result. If the truncated
      // argument is true, then only truncated columns are extracted. Return
      // true if all the data was extracted successfully and false if one or
      // more columns were truncated.
      //
      static bool
      bind_result (bind*,
                   std::size_t count,
                   PGresult*,
                   std::size_t row,
                   bool truncated = false);

    protected:
      statement (connection&,
                 const std::string& name,
                 const std::string& stmt,
                 const Oid* types,
                 std::size_t types_count);

    protected:
      connection& conn_;
      std::string name_;

    private:
      bool deallocated_;
    };

    class LIBODB_PGSQL_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      select_statement (connection& conn,
                        const std::string& name,
                        const std::string& stmt,
                        const Oid* types,
                        std::size_t types_count,
                        binding& cond,
                        native_binding& native_cond,
                        binding& data);

      select_statement (connection& conn,
                        const std::string& name,
                        const std::string& stmt,
                        const Oid* types,
                        std::size_t types_count,
                        native_binding& native_cond,
                        binding& data);

      // Common select interface expected by the generated code.
      //
    public:
      enum result
      {
        success,
        no_data,
        truncated
      };

      void
      execute ();

      void
      cache () const
      {
      }

      std::size_t
      result_size () const
      {
        return row_count_;
      }

      // Load next row columns into bound buffers.
      //
      result
      fetch ()
      {
        return next () ? load () : no_data;
      }

      // Reload truncated columns into bound buffers.
      //
      void
      refetch ()
      {
        reload ();
      }

      // Free the result set.
      //
      void
      free_result ();

      // Finer grained control of PostgreSQL-specific interface that
      // splits fetch() into next() and load().
      //
    public:
      bool
      next ();

      result
      load ();

      void
      reload ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      binding* cond_;
      native_binding& native_cond_;

      binding& data_;

      result_ptr result_;
      std::size_t row_count_;
      std::size_t current_row_;
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
                        std::size_t types_count,
                        binding& data,
                        native_binding& native_data);

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
      binding& data_;
      native_binding& native_data_;

      bool id_cached_;
      unsigned long long id_;
    };

    class LIBODB_PGSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      // Asssumes that cond.values, cond.lengths, and cond.formats are
      // suffixes of data.values, data.lengths, and data.formats
      // respectively.
      //
      update_statement (connection& conn,
                        const std::string& name,
                        const std::string& stmt,
                        const Oid* types,
                        std::size_t types_count,
                        binding& cond,
                        native_binding& native_cond,
                        binding& data,
                        native_binding& native_data);

      void
      execute ();

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);

    private:
      binding& cond_;
      native_binding& native_cond_;

      binding& data_;
      native_binding& native_data_;
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
                        std::size_t types_count,
                        binding& cond,
                        native_binding& native_cond);

      unsigned long long
      execute ();

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);

    private:
      binding& cond_;
      native_binding& native_cond_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_PGSQL_STATEMENT_HXX
