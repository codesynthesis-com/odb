// file      : odb/mssql/statement.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MSSQL_STATEMENT_HXX
#define ODB_MSSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/forward.hxx>
#include <odb/statement.hxx>

#include <odb/mssql/mssql-fwd.hxx>
#include <odb/mssql/version.hxx>
#include <odb/mssql/binding.hxx>
#include <odb/mssql/auto-handle.hxx>

#include <odb/mssql/details/export.hxx>

namespace odb
{
  namespace mssql
  {
    class connection;

    class LIBODB_MSSQL_EXPORT statement: public odb::statement
    {
    public:
      virtual
      ~statement () = 0;

      SQLHSTMT
      handle () const
      {
        return stmt_;
      }

      virtual const char*
      text () const;

    protected:
      statement (connection&, const std::string& text);
      statement (connection&, const char* text, bool copy_text);

    private:
      void
      init (std::size_t text_size);

    protected:
      void
      bind_param (bind*, std::size_t count);

      std::size_t
      bind_result (bind*, std::size_t count);

      SQLRETURN
      execute ();

      void
      stream_result (bind*, std::size_t start, std::size_t count);

    protected:
      connection& conn_;
      std::string text_copy_;
      const char* text_;
      auto_handle<SQL_HANDLE_STMT> stmt_;
    };

    class LIBODB_MSSQL_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      // While the long data columns can appear in any order in the
      // result binding, they should appear last in the statement
      // text.
      //
      select_statement (connection& conn,
                        const std::string& text,
                        binding& param,
                        binding& result);

      select_statement (connection& conn,
                        const char* text,
                        binding& param,
                        binding& result,
                        bool copy_text = true);

      select_statement (connection& conn,
                        const std::string& text,
                        binding& result);

      select_statement (connection& conn,
                        const char* text,
                        binding& result,
                        bool copy_text = true);

      enum result
      {
        success,
        no_data
      };

      void
      execute ();

      result
      fetch ();

      void
      stream_result ()
      {
        if (first_long_ != result_.count)
          statement::stream_result (result_.bind, first_long_, result_.count);
      }

      void
      free_result ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      binding& result_;
      std::size_t first_long_; // First long data column.
      bool executed_;
    };

    class LIBODB_MSSQL_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection& conn,
                        const std::string& text,
                        binding& param,
                        bool returning);

      insert_statement (connection& conn,
                        const char* text,
                        binding& param,
                        bool returning,
                        bool copy_text = true);

      // Return true if successful and false if the row is a duplicate.
      // All other errors are reported by throwing exceptions.
      //
      bool
      execute ();

      unsigned long long
      id ()
      {
        return id_;
      }

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

    private:
      void
      init_result ();

    private:
      bool returning_;
      unsigned long long id_;
      SQLLEN id_size_ind_;
    };

    class LIBODB_MSSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection& conn,
                        const std::string& text,
                        binding& param);

      update_statement (connection& conn,
                        const char* text,
                        binding& param,
                        bool copy_text = true);

      unsigned long long
      execute ();

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);
    };

    class LIBODB_MSSQL_EXPORT delete_statement: public statement
    {
    public:
      virtual
      ~delete_statement ();

      delete_statement (connection& conn,
                        const std::string& text,
                        binding& param);

      delete_statement (connection& conn,
                        const char* text,
                        binding& param,
                        bool copy_text = true);

      unsigned long long
      execute ();

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MSSQL_STATEMENT_HXX
