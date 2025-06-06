// -*- C++ -*-
//
// This file was generated by CLI, a command line interface
// compiler for C++.
//

#ifndef LIBODB_MSSQL_DETAILS_OPTIONS_HXX
#define LIBODB_MSSQL_DETAILS_OPTIONS_HXX

// Begin prologue.
//
//
// End prologue.

#include <list>
#include <deque>
#include <iosfwd>
#include <string>
#include <cstddef>
#include <exception>

#ifndef CLI_POTENTIALLY_UNUSED
#  if defined(_MSC_VER) || defined(__xlC__)
#    define CLI_POTENTIALLY_UNUSED(x) (void*)&x
#  else
#    define CLI_POTENTIALLY_UNUSED(x) (void)x
#  endif
#endif

namespace odb
{
  namespace mssql
  {
    namespace details
    {
      namespace cli
      {
        class usage_para
        {
          public:
          enum value
          {
            none,
            text,
            option
          };

          usage_para (value);

          operator value () const 
          {
            return v_;
          }

          private:
          value v_;
        };

        class unknown_mode
        {
          public:
          enum value
          {
            skip,
            stop,
            fail
          };

          unknown_mode (value);

          operator value () const 
          {
            return v_;
          }

          private:
          value v_;
        };

        // Exceptions.
        //

        class exception: public std::exception
        {
          public:
          virtual void
          print (::std::ostream&) const = 0;
        };

        ::std::ostream&
        operator<< (::std::ostream&, const exception&);

        class unknown_option: public exception
        {
          public:
          virtual
          ~unknown_option () throw ();

          unknown_option (const std::string& option);

          const std::string&
          option () const;

          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();

          private:
          std::string option_;
        };

        class unknown_argument: public exception
        {
          public:
          virtual
          ~unknown_argument () throw ();

          unknown_argument (const std::string& argument);

          const std::string&
          argument () const;

          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();

          private:
          std::string argument_;
        };

        class missing_value: public exception
        {
          public:
          virtual
          ~missing_value () throw ();

          missing_value (const std::string& option);

          const std::string&
          option () const;

          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();

          private:
          std::string option_;
        };

        class invalid_value: public exception
        {
          public:
          virtual
          ~invalid_value () throw ();

          invalid_value (const std::string& option,
                         const std::string& value,
                         const std::string& message = std::string ());

          const std::string&
          option () const;

          const std::string&
          value () const;

          const std::string&
          message () const;

          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();

          private:
          std::string option_;
          std::string value_;
          std::string message_;
        };

        class eos_reached: public exception
        {
          public:
          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();
        };

        class file_io_failure: public exception
        {
          public:
          virtual
          ~file_io_failure () throw ();

          file_io_failure (const std::string& file);

          const std::string&
          file () const;

          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();

          private:
          std::string file_;
        };

        class unmatched_quote: public exception
        {
          public:
          virtual
          ~unmatched_quote () throw ();

          unmatched_quote (const std::string& argument);

          const std::string&
          argument () const;

          virtual void
          print (::std::ostream&) const;

          virtual const char*
          what () const throw ();

          private:
          std::string argument_;
        };

        // Command line argument scanner interface.
        //
        // The values returned by next() are guaranteed to be valid
        // for the two previous arguments up until a call to a third
        // peek() or next().
        //
        // The position() function returns a monotonically-increasing
        // number which, if stored, can later be used to determine the
        // relative position of the argument returned by the following
        // call to next(). Note that if multiple scanners are used to
        // extract arguments from multiple sources, then the end
        // position of the previous scanner should be used as the
        // start position of the next.
        //
        class scanner
        {
          public:
          virtual
          ~scanner ();

          virtual bool
          more () = 0;

          virtual const char*
          peek () = 0;

          virtual const char*
          next () = 0;

          virtual void
          skip () = 0;

          virtual std::size_t
          position () = 0;
        };

        class argv_scanner: public scanner
        {
          public:
          argv_scanner (int& argc,
                        char** argv,
                        bool erase = false,
                        std::size_t start_position = 0);

          argv_scanner (int start,
                        int& argc,
                        char** argv,
                        bool erase = false,
                        std::size_t start_position = 0);

          int
          end () const;

          virtual bool
          more ();

          virtual const char*
          peek ();

          virtual const char*
          next ();

          virtual void
          skip ();

          virtual std::size_t
          position ();

          protected:
          std::size_t start_position_;
          int i_;
          int& argc_;
          char** argv_;
          bool erase_;
        };

        class argv_file_scanner: public argv_scanner
        {
          public:
          argv_file_scanner (int& argc,
                             char** argv,
                             const std::string& option,
                             bool erase = false,
                             std::size_t start_position = 0);

          argv_file_scanner (int start,
                             int& argc,
                             char** argv,
                             const std::string& option,
                             bool erase = false,
                             std::size_t start_position = 0);

          argv_file_scanner (const std::string& file,
                             const std::string& option,
                             std::size_t start_position = 0);

          struct option_info
          {
            // If search_func is not NULL, it is called, with the arg
            // value as the second argument, to locate the options file.
            // If it returns an empty string, then the file is ignored.
            //
            const char* option;
            std::string (*search_func) (const char*, void* arg);
            void* arg;
          };

          argv_file_scanner (int& argc,
                              char** argv,
                              const option_info* options,
                              std::size_t options_count,
                              bool erase = false,
                              std::size_t start_position = 0);

          argv_file_scanner (int start,
                             int& argc,
                             char** argv,
                             const option_info* options,
                             std::size_t options_count,
                             bool erase = false,
                             std::size_t start_position = 0);

          argv_file_scanner (const std::string& file,
                             const option_info* options = 0,
                             std::size_t options_count = 0,
                             std::size_t start_position = 0);

          virtual bool
          more ();

          virtual const char*
          peek ();

          virtual const char*
          next ();

          virtual void
          skip ();

          virtual std::size_t
          position ();

          // Return the file path if the peeked at argument came from a file and
          // the empty string otherwise. The reference is guaranteed to be valid
          // till the end of the scanner lifetime.
          //
          const std::string&
          peek_file ();

          // Return the 1-based line number if the peeked at argument came from
          // a file and zero otherwise.
          //
          std::size_t
          peek_line ();

          private:
          const option_info*
          find (const char*) const;

          void
          load (const std::string& file);

          typedef argv_scanner base;

          const std::string option_;
          option_info option_info_;
          const option_info* options_;
          std::size_t options_count_;

          struct arg
          {
            std::string value;
            const std::string* file;
            std::size_t line;
          };

          std::deque<arg> args_;
          std::list<std::string> files_;

          // Circular buffer of two arguments.
          //
          std::string hold_[2];
          std::size_t i_;

          bool skip_;

          static int zero_argc_;
          static std::string empty_string_;
        };

        template <typename X>
        struct parser;
      }
    }
  }
}

#include <string>

namespace odb
{
  namespace mssql
  {
    namespace details
    {
      class options
      {
        public:
        options ();

        options (int& argc,
                 char** argv,
                 bool erase = false,
                 ::odb::mssql::details::cli::unknown_mode option = ::odb::mssql::details::cli::unknown_mode::fail,
                 ::odb::mssql::details::cli::unknown_mode argument = ::odb::mssql::details::cli::unknown_mode::stop);

        options (int start,
                 int& argc,
                 char** argv,
                 bool erase = false,
                 ::odb::mssql::details::cli::unknown_mode option = ::odb::mssql::details::cli::unknown_mode::fail,
                 ::odb::mssql::details::cli::unknown_mode argument = ::odb::mssql::details::cli::unknown_mode::stop);

        options (int& argc,
                 char** argv,
                 int& end,
                 bool erase = false,
                 ::odb::mssql::details::cli::unknown_mode option = ::odb::mssql::details::cli::unknown_mode::fail,
                 ::odb::mssql::details::cli::unknown_mode argument = ::odb::mssql::details::cli::unknown_mode::stop);

        options (int start,
                 int& argc,
                 char** argv,
                 int& end,
                 bool erase = false,
                 ::odb::mssql::details::cli::unknown_mode option = ::odb::mssql::details::cli::unknown_mode::fail,
                 ::odb::mssql::details::cli::unknown_mode argument = ::odb::mssql::details::cli::unknown_mode::stop);

        options (::odb::mssql::details::cli::scanner&,
                 ::odb::mssql::details::cli::unknown_mode option = ::odb::mssql::details::cli::unknown_mode::fail,
                 ::odb::mssql::details::cli::unknown_mode argument = ::odb::mssql::details::cli::unknown_mode::stop);

        // Option accessors.
        //
        const std::string&
        user () const;

        bool
        user_specified () const;

        const std::string&
        password () const;

        bool
        password_specified () const;

        const std::string&
        database () const;

        bool
        database_specified () const;

        const std::string&
        server () const;

        bool
        server_specified () const;

        const std::string&
        driver () const;

        bool
        driver_specified () const;

        const std::string&
        options_file () const;

        bool
        options_file_specified () const;

        // Print usage information.
        //
        static ::odb::mssql::details::cli::usage_para
        print_usage (::std::ostream&,
                     ::odb::mssql::details::cli::usage_para = ::odb::mssql::details::cli::usage_para::none);

        // Implementation details.
        //
        protected:
        bool
        _parse (const char*, ::odb::mssql::details::cli::scanner&);

        private:
        bool
        _parse (::odb::mssql::details::cli::scanner&,
                ::odb::mssql::details::cli::unknown_mode option,
                ::odb::mssql::details::cli::unknown_mode argument);

        public:
        std::string user_;
        bool user_specified_;
        std::string password_;
        bool password_specified_;
        std::string database_;
        bool database_specified_;
        std::string server_;
        bool server_specified_;
        std::string driver_;
        bool driver_specified_;
        std::string options_file_;
        bool options_file_specified_;
      };
    }
  }
}

#include <odb/mssql/details/options.ixx>

// Begin epilogue.
//
//
// End epilogue.

#endif // LIBODB_MSSQL_DETAILS_OPTIONS_HXX
