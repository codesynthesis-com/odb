// file      : odb/mysql/statement.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_MYSQL_STATEMENT_HXX
#define ODB_MYSQL_STATEMENT_HXX

#include <odb/pre.hxx>

#include <string>
#include <cstddef>  // std::size_t

#include <odb/statement.hxx>

#include <odb/mysql/mysql.hxx>
#include <odb/mysql/version.hxx>
#include <odb/mysql/forward.hxx>
#include <odb/mysql/binding.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/auto-handle.hxx>

#include <odb/mysql/details/export.hxx>

namespace odb
{
  namespace mysql
  {
    class connection;

    class LIBODB_MYSQL_EXPORT statement: public odb::statement
    {
    public:
      typedef mysql::connection connection_type;

      virtual
      ~statement () = 0;

      MYSQL_STMT*
      handle () const
      {
        return stmt_;
      }

      virtual const char*
      text () const;

      virtual connection_type&
      connection ()
      {
        return conn_;
      }

      // A statement can be empty. This is used to handle situations
      // where a SELECT or UPDATE statement ends up not having any
      // columns after processing. An empty statement cannot be
      // executed.
      //
      bool
      empty () const
      {
        return stmt_ == 0;
      }

      // Cancel the statement execution (e.g., result fetching) so
      // that another statement can be executed on the connection.
      //
      virtual void
      cancel ();

    protected:
      // We keep two versions to take advantage of std::string COW.
      //
      statement (connection_type&,
                 const std::string& text,
                 statement_kind,
                 const binding* process,
                 bool optimize);

      statement (connection_type&,
                 const char* text,
                 statement_kind,
                 const binding* process,
                 bool optimize,
                 bool copy_text);

      // Process the bind array so that all non-NULL entries are at
      // the beginning. Return the actual number of bound columns.
      //
      static std::size_t
      process_bind (MYSQL_BIND*, std::size_t n);

      // Restore the original locations of the NULL entries in the bind
      // array.
      //
      static void
      restore_bind (MYSQL_BIND*, std::size_t n);

    private:
      void
      init (std::size_t text_size,
            statement_kind,
            const binding* process,
            bool optimize);

    protected:
      connection_type& conn_;
      std::string text_copy_;
      const char* text_;
      auto_handle<MYSQL_STMT> stmt_;
    };

    class LIBODB_MYSQL_EXPORT select_statement: public statement
    {
    public:
      virtual
      ~select_statement ();

      select_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& param,
                        binding& result);

      select_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& param,
                        binding& result,
                        bool copy_text = true);

      select_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        bool optimize_text,
                        binding& result);

      select_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        bool optimize_text,
                        binding& result,
                        bool copy_text = true);

      enum result
      {
        success,
        no_data,
        truncated
      };

      void
      execute ();

      void
      cache ();

      bool
      cached () const
      {
        return cached_;
      }

      // Can only be called on a cached result.
      //
      std::size_t
      result_size () const
      {
        return size_;
      }

      // Number of rows already fetched.
      //
      std::size_t
      fetched () const
      {
        return rows_;
      }

      // Fetch next or current row depending on the next argument.
      // Note that fetching of the current row is only supported
      // if the result is cached.
      //
      result
      fetch (bool next = true);

      void
      refetch ();

      void
      free_result ();

      virtual void
      cancel ();

    private:
      select_statement (const select_statement&);
      select_statement& operator= (const select_statement&);

    private:
      bool end_;
      bool cached_;
      bool freed_;
      std::size_t rows_;
      std::size_t size_;

#if MYSQL_VERSION_ID >= 50503
      bool out_params_;
#endif

      binding* param_;
      std::size_t param_version_;

      binding& result_;
      std::size_t result_version_;
    };

    struct LIBODB_MYSQL_EXPORT auto_result
    {
      explicit auto_result (select_statement& s): s_ (s) {}
      ~auto_result () {s_.free_result ();}

    private:
      auto_result (const auto_result&);
      auto_result& operator= (const auto_result&);

    private:
      select_statement& s_;
    };

    class LIBODB_MYSQL_EXPORT insert_statement: public statement
    {
    public:
      virtual
      ~insert_statement ();

      insert_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        binding& param,
                        binding* returning);

      insert_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        binding& param,
                        binding* returning,
                        bool copy_text = true);

      // Return true if successful and false if the row is a duplicate.
      // All other errors are reported by throwing exceptions.
      //
      bool
      execute ();

    private:
      insert_statement (const insert_statement&);
      insert_statement& operator= (const insert_statement&);

    private:
      binding& param_;
      std::size_t param_version_;

      binding* returning_;
    };

    class LIBODB_MYSQL_EXPORT update_statement: public statement
    {
    public:
      virtual
      ~update_statement ();

      update_statement (connection_type& conn,
                        const std::string& text,
                        bool process_text,
                        binding& param);

      update_statement (connection_type& conn,
                        const char* text,
                        bool process_text,
                        binding& param,
                        bool copy_text = true);

      unsigned long long
      execute ();

    private:
      update_statement (const update_statement&);
      update_statement& operator= (const update_statement&);

    private:
      binding& param_;
      std::size_t param_version_;
    };

    class LIBODB_MYSQL_EXPORT delete_statement: public statement
    {
    public:
      virtual
      ~delete_statement ();

      delete_statement (connection_type& conn,
                        const std::string& text,
                        binding& param);

      delete_statement (connection_type& conn,
                        const char* text,
                        binding& param,
                        bool copy_text = true);

      unsigned long long
      execute ();

    private:
      delete_statement (const delete_statement&);
      delete_statement& operator= (const delete_statement&);

    private:
      binding& param_;
      std::size_t param_version_;
    };
  }
}

#include <odb/post.hxx>

#endif // ODB_MYSQL_STATEMENT_HXX
