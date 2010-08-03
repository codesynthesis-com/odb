// This code was generated by CLI, a command line interface
// compiler for C++.
//

#include <odb/options.hxx>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include <cstring>
#include <fstream>

namespace cli
{
  // unknown_option
  //
  unknown_option::
  ~unknown_option () throw ()
  {
  }

  void unknown_option::
  print (std::ostream& os) const
  {
    os << "unknown option '" << option () << "'";
  }

  const char* unknown_option::
  what () const throw ()
  {
    return "unknown option";
  }

  // unknown_argument
  //
  unknown_argument::
  ~unknown_argument () throw ()
  {
  }

  void unknown_argument::
  print (std::ostream& os) const
  {
    os << "unknown argument '" << argument () << "'";
  }

  const char* unknown_argument::
  what () const throw ()
  {
    return "unknown argument";
  }

  // missing_value
  //
  missing_value::
  ~missing_value () throw ()
  {
  }

  void missing_value::
  print (std::ostream& os) const
  {
    os << "missing value for option '" << option () << "'";
  }

  const char* missing_value::
  what () const throw ()
  {
    return "missing option value";
  }

  // invalid_value
  //
  invalid_value::
  ~invalid_value () throw ()
  {
  }

  void invalid_value::
  print (std::ostream& os) const
  {
    os << "invalid value '" << value () << "' for option '"
       << option () << "'";
  }

  const char* invalid_value::
  what () const throw ()
  {
    return "invalid option value";
  }

  // eos_reached
  //
  void eos_reached::
  print (std::ostream& os) const
  {
    os << what ();
  }

  const char* eos_reached::
  what () const throw ()
  {
    return "end of argument stream reached";
  }

  // file_io_failure
  //
  file_io_failure::
  ~file_io_failure () throw ()
  {
  }

  void file_io_failure::
  print (std::ostream& os) const
  {
    os << "unable to open file '" << file () << "' or read failure";
  }

  const char* file_io_failure::
  what () const throw ()
  {
    return "unable to open file or read failure";
  }

  // unmatched_quote
  //
  unmatched_quote::
  ~unmatched_quote () throw ()
  {
  }

  void unmatched_quote::
  print (std::ostream& os) const
  {
    os << "unmatched quote in argument '" << argument () << "'";
  }

  const char* unmatched_quote::
  what () const throw ()
  {
    return "unmatched quote";
  }

  // scanner
  //
  scanner::
  ~scanner ()
  {
  }

  // argv_scanner
  //
  bool argv_scanner::
  more ()
  {
    return i_ < argc_;
  }

  const char* argv_scanner::
  peek ()
  {
    if (i_ < argc_)
      return argv_[i_];
    else
      throw eos_reached ();
  }

  const char* argv_scanner::
  next ()
  {
    if (i_ < argc_)
    {
      const char* r (argv_[i_]);

      if (erase_)
      {
        for (int i (i_ + 1); i < argc_; ++i)
          argv_[i - 1] = argv_[i];

        --argc_;
        argv_[argc_] = 0;
      }
      else
        ++i_;

      return r;
    }
    else
      throw eos_reached ();
  }

  void argv_scanner::
  skip ()
  {
    if (i_ < argc_)
      ++i_;
    else
      throw eos_reached ();
  }

  // argv_file_scanner
  //
  bool argv_file_scanner::
  more ()
  {
    if (!args_.empty ())
      return true;

    while (base::more ())
    {
      // See if the next argument is the file option.
      //
      const char* a (base::peek ());

      if (!skip_ && a == option_)
      {
        base::next ();

        if (!base::more ())
          throw missing_value (option_);

        load (base::next ());

        if (!args_.empty ())
          return true;
      }
      else
      {
        if (!skip_)
          skip_ = (std::strcmp (a, "--") == 0);

        return true;
      }
    }

    return false;
  }

  const char* argv_file_scanner::
  peek ()
  {
    if (!more ())
      throw eos_reached ();

    return args_.empty () ? base::peek () : args_.front ().c_str ();
  }

  const char* argv_file_scanner::
  next ()
  {
    if (!more ())
      throw eos_reached ();

    if (args_.empty ())
      return base::next ();
    else
    {
      hold_.swap (args_.front ());
      args_.pop_front ();
      return hold_.c_str ();
    }
  }

  void argv_file_scanner::
  skip ()
  {
    if (!more ())
      throw eos_reached ();

    if (args_.empty ())
      return base::skip ();
    else
      args_.pop_front ();
  }

  void argv_file_scanner::
  load (const char* file)
  {
    using namespace std;

    ifstream is (file);

    if (!is.is_open ())
      throw file_io_failure (file);

    while (!is.eof ())
    {
      string line;
      getline (is, line);

      if (is.fail () && !is.eof ())
        throw file_io_failure (file);

      string::size_type n (line.size ());

      // Trim the line from leading and trailing whitespaces.
      //
      if (n != 0)
      {
        const char* f (line.c_str ());
        const char* l (f + n);

        const char* of (f);
        while (f < l && (*f == ' ' || *f == '\t' || *f == '\r'))
          ++f;

        --l;

        const char* ol (l);
        while (l > f && (*l == ' ' || *l == '\t' || *l == '\r'))
          --l;

        if (f != of || l != ol)
          line = f <= l ? string (f, l - f + 1) : string ();
      }

      // Ignore empty lines, those that start with #.
      //
      if (line.empty () || line[0] == '#')
        continue;

      string::size_type p (line.find (' '));

      if (p == string::npos)
      {
        if (!skip_)
          skip_ = (line == "--");

        args_.push_back (line);
      }
      else
      {
        string s1 (line, 0, p);

        // Skip leading whitespaces in the argument.
        //
        n = line.size ();
        for (++p; p < n; ++p)
        {
          char c (line[p]);

          if (c != ' ' && c != '\t' && c != '\r')
            break;
        }

        string s2 (line, p);

        // If the string is wrapped in quotes, remove them.
        //
        n = s2.size ();
        char cf (s2[0]), cl (s2[n - 1]);

        if (cf == '"' || cf == '\'' || cl == '"' || cl == '\'')
        {
          if (n == 1 || cf != cl)
            throw unmatched_quote (s2);

          s2 = string (s2, 1, n - 2);
        }

        if (!skip_ && s1 == option_)
          load (s2.c_str ());
        else
        {
          args_.push_back (s1);
          args_.push_back (s2);
        }
      }
    }
  }

  template <typename X>
  struct parser
  {
    static void
    parse (X& x, bool& xs, scanner& s)
    {
      const char* o (s.next ());

      if (s.more ())
      {
        const char* v (s.next ());
        std::istringstream is (v);
        if (!(is >> x && is.eof ()))
          throw invalid_value (o, v);
      }
      else
        throw missing_value (o);

      xs = true;
    }
  };

  template <>
  struct parser<bool>
  {
    static void
    parse (bool& x, scanner& s)
    {
      s.next ();
      x = true;
    }
  };

  template <>
  struct parser<std::string>
  {
    static void
    parse (std::string& x, bool& xs, scanner& s)
    {
      const char* o (s.next ());

      if (s.more ())
        x = s.next ();
      else
        throw missing_value (o);

      xs = true;
    }
  };

  template <typename X>
  struct parser<std::vector<X> >
  {
    static void
    parse (std::vector<X>& c, bool& xs, scanner& s)
    {
      X x;
      bool dummy;
      parser<X>::parse (x, dummy, s);
      c.push_back (x);
      xs = true;
    }
  };

  template <typename X>
  struct parser<std::set<X> >
  {
    static void
    parse (std::set<X>& c, bool& xs, scanner& s)
    {
      X x;
      bool dummy;
      parser<X>::parse (x, dummy, s);
      c.insert (x);
      xs = true;
    }
  };

  template <typename K, typename V>
  struct parser<std::map<K, V> >
  {
    static void
    parse (std::map<K, V>& m, bool& xs, scanner& s)
    {
      const char* o (s.next ());

      if (s.more ())
      {
        std::string ov (s.next ());
        std::string::size_type p = ov.find ('=');

        if (p == std::string::npos)
        {
          K k = K ();

          if (!ov.empty ())
          {
            std::istringstream ks (ov);

            if (!(ks >> k && ks.eof ()))
              throw invalid_value (o, ov);
          }

          m[k] = V ();
        }
        else
        {
          K k = K ();
          V v = V ();
          std::string kstr (ov, 0, p);
          std::string vstr (ov, p + 1);

          if (!kstr.empty ())
          {
            std::istringstream ks (kstr);

            if (!(ks >> k && ks.eof ()))
              throw invalid_value (o, ov);
          }

          if (!vstr.empty ())
          {
            std::istringstream vs (vstr);

            if (!(vs >> v && vs.eof ()))
              throw invalid_value (o, ov);
          }

          m[k] = v;
        }
      }
      else
        throw missing_value (o);

      xs = true;
    }
  };

  template <typename X, typename T, T X::*M>
  void
  thunk (X& x, scanner& s)
  {
    parser<T>::parse (x.*M, s);
  }

  template <typename X, typename T, T X::*M, bool X::*S>
  void
  thunk (X& x, scanner& s)
  {
    parser<T>::parse (x.*M, x.*S, s);
  }
}

#include <map>
#include <cstring>

// options
//

options::
options (int& argc,
         char** argv,
         bool erase,
         ::cli::unknown_mode opt,
         ::cli::unknown_mode arg)
: help_ (),
  version_ (),
  database_ (),
  database_specified_ (false),
  generate_schema_ (),
  output_dir_ (),
  output_dir_specified_ (false),
  odb_file_suffix_ ("-odb"),
  odb_file_suffix_specified_ (false),
  hxx_suffix_ (".hxx"),
  hxx_suffix_specified_ (false),
  ixx_suffix_ (".ixx"),
  ixx_suffix_specified_ (false),
  cxx_suffix_ (".cxx"),
  cxx_suffix_specified_ (false),
  sql_suffix_ (".sql"),
  sql_suffix_specified_ (false),
  hxx_prologue_ (),
  hxx_prologue_specified_ (false),
  ixx_prologue_ (),
  ixx_prologue_specified_ (false),
  cxx_prologue_ (),
  cxx_prologue_specified_ (false),
  sql_prologue_ (),
  sql_prologue_specified_ (false),
  hxx_epilogue_ (),
  hxx_epilogue_specified_ (false),
  ixx_epilogue_ (),
  ixx_epilogue_specified_ (false),
  cxx_epilogue_ (),
  cxx_epilogue_specified_ (false),
  sql_epilogue_ (),
  sql_epilogue_specified_ (false),
  hxx_prologue_file_ (),
  hxx_prologue_file_specified_ (false),
  ixx_prologue_file_ (),
  ixx_prologue_file_specified_ (false),
  cxx_prologue_file_ (),
  cxx_prologue_file_specified_ (false),
  sql_prologue_file_ (),
  sql_prologue_file_specified_ (false),
  hxx_epilogue_file_ (),
  hxx_epilogue_file_specified_ (false),
  ixx_epilogue_file_ (),
  ixx_epilogue_file_specified_ (false),
  cxx_epilogue_file_ (),
  cxx_epilogue_file_specified_ (false),
  sql_epilogue_file_ (),
  sql_epilogue_file_specified_ (false),
  include_with_brackets_ (),
  include_prefix_ (),
  include_prefix_specified_ (false),
  guard_prefix_ (),
  guard_prefix_specified_ (false),
  options_file_ (),
  options_file_specified_ (false),
  trace_ (),
  mysql_engine_ ("InnoDB"),
  mysql_engine_specified_ (false)
{
  ::cli::argv_scanner s (argc, argv, erase);
  _parse (s, opt, arg);
}

options::
options (int start,
         int& argc,
         char** argv,
         bool erase,
         ::cli::unknown_mode opt,
         ::cli::unknown_mode arg)
: help_ (),
  version_ (),
  database_ (),
  database_specified_ (false),
  generate_schema_ (),
  output_dir_ (),
  output_dir_specified_ (false),
  odb_file_suffix_ ("-odb"),
  odb_file_suffix_specified_ (false),
  hxx_suffix_ (".hxx"),
  hxx_suffix_specified_ (false),
  ixx_suffix_ (".ixx"),
  ixx_suffix_specified_ (false),
  cxx_suffix_ (".cxx"),
  cxx_suffix_specified_ (false),
  sql_suffix_ (".sql"),
  sql_suffix_specified_ (false),
  hxx_prologue_ (),
  hxx_prologue_specified_ (false),
  ixx_prologue_ (),
  ixx_prologue_specified_ (false),
  cxx_prologue_ (),
  cxx_prologue_specified_ (false),
  sql_prologue_ (),
  sql_prologue_specified_ (false),
  hxx_epilogue_ (),
  hxx_epilogue_specified_ (false),
  ixx_epilogue_ (),
  ixx_epilogue_specified_ (false),
  cxx_epilogue_ (),
  cxx_epilogue_specified_ (false),
  sql_epilogue_ (),
  sql_epilogue_specified_ (false),
  hxx_prologue_file_ (),
  hxx_prologue_file_specified_ (false),
  ixx_prologue_file_ (),
  ixx_prologue_file_specified_ (false),
  cxx_prologue_file_ (),
  cxx_prologue_file_specified_ (false),
  sql_prologue_file_ (),
  sql_prologue_file_specified_ (false),
  hxx_epilogue_file_ (),
  hxx_epilogue_file_specified_ (false),
  ixx_epilogue_file_ (),
  ixx_epilogue_file_specified_ (false),
  cxx_epilogue_file_ (),
  cxx_epilogue_file_specified_ (false),
  sql_epilogue_file_ (),
  sql_epilogue_file_specified_ (false),
  include_with_brackets_ (),
  include_prefix_ (),
  include_prefix_specified_ (false),
  guard_prefix_ (),
  guard_prefix_specified_ (false),
  options_file_ (),
  options_file_specified_ (false),
  trace_ (),
  mysql_engine_ ("InnoDB"),
  mysql_engine_specified_ (false)
{
  ::cli::argv_scanner s (start, argc, argv, erase);
  _parse (s, opt, arg);
}

options::
options (int& argc,
         char** argv,
         int& end,
         bool erase,
         ::cli::unknown_mode opt,
         ::cli::unknown_mode arg)
: help_ (),
  version_ (),
  database_ (),
  database_specified_ (false),
  generate_schema_ (),
  output_dir_ (),
  output_dir_specified_ (false),
  odb_file_suffix_ ("-odb"),
  odb_file_suffix_specified_ (false),
  hxx_suffix_ (".hxx"),
  hxx_suffix_specified_ (false),
  ixx_suffix_ (".ixx"),
  ixx_suffix_specified_ (false),
  cxx_suffix_ (".cxx"),
  cxx_suffix_specified_ (false),
  sql_suffix_ (".sql"),
  sql_suffix_specified_ (false),
  hxx_prologue_ (),
  hxx_prologue_specified_ (false),
  ixx_prologue_ (),
  ixx_prologue_specified_ (false),
  cxx_prologue_ (),
  cxx_prologue_specified_ (false),
  sql_prologue_ (),
  sql_prologue_specified_ (false),
  hxx_epilogue_ (),
  hxx_epilogue_specified_ (false),
  ixx_epilogue_ (),
  ixx_epilogue_specified_ (false),
  cxx_epilogue_ (),
  cxx_epilogue_specified_ (false),
  sql_epilogue_ (),
  sql_epilogue_specified_ (false),
  hxx_prologue_file_ (),
  hxx_prologue_file_specified_ (false),
  ixx_prologue_file_ (),
  ixx_prologue_file_specified_ (false),
  cxx_prologue_file_ (),
  cxx_prologue_file_specified_ (false),
  sql_prologue_file_ (),
  sql_prologue_file_specified_ (false),
  hxx_epilogue_file_ (),
  hxx_epilogue_file_specified_ (false),
  ixx_epilogue_file_ (),
  ixx_epilogue_file_specified_ (false),
  cxx_epilogue_file_ (),
  cxx_epilogue_file_specified_ (false),
  sql_epilogue_file_ (),
  sql_epilogue_file_specified_ (false),
  include_with_brackets_ (),
  include_prefix_ (),
  include_prefix_specified_ (false),
  guard_prefix_ (),
  guard_prefix_specified_ (false),
  options_file_ (),
  options_file_specified_ (false),
  trace_ (),
  mysql_engine_ ("InnoDB"),
  mysql_engine_specified_ (false)
{
  ::cli::argv_scanner s (argc, argv, erase);
  _parse (s, opt, arg);
  end = s.end ();
}

options::
options (int start,
         int& argc,
         char** argv,
         int& end,
         bool erase,
         ::cli::unknown_mode opt,
         ::cli::unknown_mode arg)
: help_ (),
  version_ (),
  database_ (),
  database_specified_ (false),
  generate_schema_ (),
  output_dir_ (),
  output_dir_specified_ (false),
  odb_file_suffix_ ("-odb"),
  odb_file_suffix_specified_ (false),
  hxx_suffix_ (".hxx"),
  hxx_suffix_specified_ (false),
  ixx_suffix_ (".ixx"),
  ixx_suffix_specified_ (false),
  cxx_suffix_ (".cxx"),
  cxx_suffix_specified_ (false),
  sql_suffix_ (".sql"),
  sql_suffix_specified_ (false),
  hxx_prologue_ (),
  hxx_prologue_specified_ (false),
  ixx_prologue_ (),
  ixx_prologue_specified_ (false),
  cxx_prologue_ (),
  cxx_prologue_specified_ (false),
  sql_prologue_ (),
  sql_prologue_specified_ (false),
  hxx_epilogue_ (),
  hxx_epilogue_specified_ (false),
  ixx_epilogue_ (),
  ixx_epilogue_specified_ (false),
  cxx_epilogue_ (),
  cxx_epilogue_specified_ (false),
  sql_epilogue_ (),
  sql_epilogue_specified_ (false),
  hxx_prologue_file_ (),
  hxx_prologue_file_specified_ (false),
  ixx_prologue_file_ (),
  ixx_prologue_file_specified_ (false),
  cxx_prologue_file_ (),
  cxx_prologue_file_specified_ (false),
  sql_prologue_file_ (),
  sql_prologue_file_specified_ (false),
  hxx_epilogue_file_ (),
  hxx_epilogue_file_specified_ (false),
  ixx_epilogue_file_ (),
  ixx_epilogue_file_specified_ (false),
  cxx_epilogue_file_ (),
  cxx_epilogue_file_specified_ (false),
  sql_epilogue_file_ (),
  sql_epilogue_file_specified_ (false),
  include_with_brackets_ (),
  include_prefix_ (),
  include_prefix_specified_ (false),
  guard_prefix_ (),
  guard_prefix_specified_ (false),
  options_file_ (),
  options_file_specified_ (false),
  trace_ (),
  mysql_engine_ ("InnoDB"),
  mysql_engine_specified_ (false)
{
  ::cli::argv_scanner s (start, argc, argv, erase);
  _parse (s, opt, arg);
  end = s.end ();
}

options::
options (::cli::scanner& s,
         ::cli::unknown_mode opt,
         ::cli::unknown_mode arg)
: help_ (),
  version_ (),
  database_ (),
  database_specified_ (false),
  generate_schema_ (),
  output_dir_ (),
  output_dir_specified_ (false),
  odb_file_suffix_ ("-odb"),
  odb_file_suffix_specified_ (false),
  hxx_suffix_ (".hxx"),
  hxx_suffix_specified_ (false),
  ixx_suffix_ (".ixx"),
  ixx_suffix_specified_ (false),
  cxx_suffix_ (".cxx"),
  cxx_suffix_specified_ (false),
  sql_suffix_ (".sql"),
  sql_suffix_specified_ (false),
  hxx_prologue_ (),
  hxx_prologue_specified_ (false),
  ixx_prologue_ (),
  ixx_prologue_specified_ (false),
  cxx_prologue_ (),
  cxx_prologue_specified_ (false),
  sql_prologue_ (),
  sql_prologue_specified_ (false),
  hxx_epilogue_ (),
  hxx_epilogue_specified_ (false),
  ixx_epilogue_ (),
  ixx_epilogue_specified_ (false),
  cxx_epilogue_ (),
  cxx_epilogue_specified_ (false),
  sql_epilogue_ (),
  sql_epilogue_specified_ (false),
  hxx_prologue_file_ (),
  hxx_prologue_file_specified_ (false),
  ixx_prologue_file_ (),
  ixx_prologue_file_specified_ (false),
  cxx_prologue_file_ (),
  cxx_prologue_file_specified_ (false),
  sql_prologue_file_ (),
  sql_prologue_file_specified_ (false),
  hxx_epilogue_file_ (),
  hxx_epilogue_file_specified_ (false),
  ixx_epilogue_file_ (),
  ixx_epilogue_file_specified_ (false),
  cxx_epilogue_file_ (),
  cxx_epilogue_file_specified_ (false),
  sql_epilogue_file_ (),
  sql_epilogue_file_specified_ (false),
  include_with_brackets_ (),
  include_prefix_ (),
  include_prefix_specified_ (false),
  guard_prefix_ (),
  guard_prefix_specified_ (false),
  options_file_ (),
  options_file_specified_ (false),
  trace_ (),
  mysql_engine_ ("InnoDB"),
  mysql_engine_specified_ (false)
{
  _parse (s, opt, arg);
}

void options::
print_usage (::std::ostream& os)
{
  os << "--help                     Print usage information and exit." << ::std::endl;

  os << "--version                  Print version and exit." << ::std::endl;

  os << "--database|-d <db>         Generate code for the <db> database." << ::std::endl;

  os << "--generate-schema          Generate database schema." << ::std::endl;

  os << "--output-dir|-o <dir>      Write the generated files to <dir> instead of the" << ::std::endl
     << "                           current directory." << ::std::endl;

  os << "--odb-file-suffix <suffix> Use <suffix> instead of the default '-odb' to" << ::std::endl
     << "                           construct the names of the generated C++ files." << ::std::endl;

  os << "--hxx-suffix <suffix>      Use <suffix> instead of the default '.hxx' to" << ::std::endl
     << "                           construct the name of the generated C++ header file." << ::std::endl;

  os << "--ixx-suffix <suffix>      Use <suffix> instead of the default '.ixx' to" << ::std::endl
     << "                           construct the name of the generated C++ inline file." << ::std::endl;

  os << "--cxx-suffix <suffix>      Use <suffix> instead of the default '.cxx' to" << ::std::endl
     << "                           construct the name of the generated C++ source file." << ::std::endl;

  os << "--sql-suffix <suffix>      Use <suffix> instead of the default '.sql' to" << ::std::endl
     << "                           construct the name of the generated database schema" << ::std::endl
     << "                           file." << ::std::endl;

  os << "--hxx-prologue <text>      Insert <text> at the beginning of the generated C++" << ::std::endl
     << "                           header file." << ::std::endl;

  os << "--ixx-prologue <text>      Insert <text> at the beginning of the generated C++" << ::std::endl
     << "                           inline file." << ::std::endl;

  os << "--cxx-prologue <text>      Insert <text> at the beginning of the generated C++" << ::std::endl
     << "                           source file." << ::std::endl;

  os << "--sql-prologue <text>      Insert <text> at the beginning of the generated" << ::std::endl
     << "                           database schema file." << ::std::endl;

  os << "--hxx-epilogue <text>      Insert <text> at the end of the generated C++ header" << ::std::endl
     << "                           file." << ::std::endl;

  os << "--ixx-epilogue <text>      Insert <text> at the end of the generated C++ inline" << ::std::endl
     << "                           file." << ::std::endl;

  os << "--cxx-epilogue <text>      Insert <text> at the end of the generated C++ source" << ::std::endl
     << "                           file." << ::std::endl;

  os << "--sql-epilogue <text>      Insert <text> at the end of the generated database" << ::std::endl
     << "                           schema file." << ::std::endl;

  os << "--hxx-prologue-file <file> Insert the content of <file> at the beginning of the" << ::std::endl
     << "                           generated C++ header file." << ::std::endl;

  os << "--ixx-prologue-file <file> Insert the content of <file> at the beginning of the" << ::std::endl
     << "                           generated C++ inline file." << ::std::endl;

  os << "--cxx-prologue-file <file> Insert the content of <file> at the beginning of the" << ::std::endl
     << "                           generated C++ source file." << ::std::endl;

  os << "--sql-prologue-file <file> Insert the content of <file> at the beginning of the" << ::std::endl
     << "                           generated database schema file." << ::std::endl;

  os << "--hxx-epilogue-file <file> Insert the content of <file> at the end of the" << ::std::endl
     << "                           generated C++ header file." << ::std::endl;

  os << "--ixx-epilogue-file <file> Insert the content of <file> at the end of the" << ::std::endl
     << "                           generated C++ inline file." << ::std::endl;

  os << "--cxx-epilogue-file <file> Insert the content of <file> at the end of the" << ::std::endl
     << "                           generated C++ source file." << ::std::endl;

  os << "--sql-epilogue-file <file> Insert the content of <file> at the end of the" << ::std::endl
     << "                           generated database schema file." << ::std::endl;

  os << "--include-with-brackets    Use angle brackets (<>) instead of quotes (\"\") in the" << ::std::endl
     << "                           generated '#include' directives." << ::std::endl;

  os << "--include-prefix <prefix>  Add <prefix> to the generated '#include' directive" << ::std::endl
     << "                           paths." << ::std::endl;

  os << "--guard-prefix <prefix>    Add <prefix> to the generated header inclusion" << ::std::endl
     << "                           guards." << ::std::endl;

  os << "--options-file <file>      Read additional options from <file> with each option" << ::std::endl
     << "                           appearing on a separate line optionally followed by" << ::std::endl
     << "                           space and an option value." << ::std::endl;

  os << "--trace                    Trace the compilation process." << ::std::endl;

  os << "--mysql-engine <engine>    Use <engine> instead of the default 'InnoDB' in the" << ::std::endl
     << "                           generated database schema file." << ::std::endl;
}

typedef
std::map<std::string, void (*) (options&, ::cli::scanner&)>
_cli_options_map;

static _cli_options_map _cli_options_map_;

struct _cli_options_map_init
{
  _cli_options_map_init ()
  {
    _cli_options_map_["--help"] = 
    &::cli::thunk< options, bool, &options::help_ >;
    _cli_options_map_["--version"] = 
    &::cli::thunk< options, bool, &options::version_ >;
    _cli_options_map_["--database"] = 
    &::cli::thunk< options, ::database, &options::database_,
      &options::database_specified_ >;
    _cli_options_map_["-d"] = 
    &::cli::thunk< options, ::database, &options::database_,
      &options::database_specified_ >;
    _cli_options_map_["--generate-schema"] = 
    &::cli::thunk< options, bool, &options::generate_schema_ >;
    _cli_options_map_["--output-dir"] = 
    &::cli::thunk< options, std::string, &options::output_dir_,
      &options::output_dir_specified_ >;
    _cli_options_map_["-o"] = 
    &::cli::thunk< options, std::string, &options::output_dir_,
      &options::output_dir_specified_ >;
    _cli_options_map_["--odb-file-suffix"] = 
    &::cli::thunk< options, std::string, &options::odb_file_suffix_,
      &options::odb_file_suffix_specified_ >;
    _cli_options_map_["--hxx-suffix"] = 
    &::cli::thunk< options, std::string, &options::hxx_suffix_,
      &options::hxx_suffix_specified_ >;
    _cli_options_map_["--ixx-suffix"] = 
    &::cli::thunk< options, std::string, &options::ixx_suffix_,
      &options::ixx_suffix_specified_ >;
    _cli_options_map_["--cxx-suffix"] = 
    &::cli::thunk< options, std::string, &options::cxx_suffix_,
      &options::cxx_suffix_specified_ >;
    _cli_options_map_["--sql-suffix"] = 
    &::cli::thunk< options, std::string, &options::sql_suffix_,
      &options::sql_suffix_specified_ >;
    _cli_options_map_["--hxx-prologue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::hxx_prologue_,
      &options::hxx_prologue_specified_ >;
    _cli_options_map_["--ixx-prologue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::ixx_prologue_,
      &options::ixx_prologue_specified_ >;
    _cli_options_map_["--cxx-prologue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::cxx_prologue_,
      &options::cxx_prologue_specified_ >;
    _cli_options_map_["--sql-prologue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::sql_prologue_,
      &options::sql_prologue_specified_ >;
    _cli_options_map_["--hxx-epilogue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::hxx_epilogue_,
      &options::hxx_epilogue_specified_ >;
    _cli_options_map_["--ixx-epilogue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::ixx_epilogue_,
      &options::ixx_epilogue_specified_ >;
    _cli_options_map_["--cxx-epilogue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::cxx_epilogue_,
      &options::cxx_epilogue_specified_ >;
    _cli_options_map_["--sql-epilogue"] = 
    &::cli::thunk< options, std::vector<std::string>, &options::sql_epilogue_,
      &options::sql_epilogue_specified_ >;
    _cli_options_map_["--hxx-prologue-file"] = 
    &::cli::thunk< options, std::string, &options::hxx_prologue_file_,
      &options::hxx_prologue_file_specified_ >;
    _cli_options_map_["--ixx-prologue-file"] = 
    &::cli::thunk< options, std::string, &options::ixx_prologue_file_,
      &options::ixx_prologue_file_specified_ >;
    _cli_options_map_["--cxx-prologue-file"] = 
    &::cli::thunk< options, std::string, &options::cxx_prologue_file_,
      &options::cxx_prologue_file_specified_ >;
    _cli_options_map_["--sql-prologue-file"] = 
    &::cli::thunk< options, std::string, &options::sql_prologue_file_,
      &options::sql_prologue_file_specified_ >;
    _cli_options_map_["--hxx-epilogue-file"] = 
    &::cli::thunk< options, std::string, &options::hxx_epilogue_file_,
      &options::hxx_epilogue_file_specified_ >;
    _cli_options_map_["--ixx-epilogue-file"] = 
    &::cli::thunk< options, std::string, &options::ixx_epilogue_file_,
      &options::ixx_epilogue_file_specified_ >;
    _cli_options_map_["--cxx-epilogue-file"] = 
    &::cli::thunk< options, std::string, &options::cxx_epilogue_file_,
      &options::cxx_epilogue_file_specified_ >;
    _cli_options_map_["--sql-epilogue-file"] = 
    &::cli::thunk< options, std::string, &options::sql_epilogue_file_,
      &options::sql_epilogue_file_specified_ >;
    _cli_options_map_["--include-with-brackets"] = 
    &::cli::thunk< options, bool, &options::include_with_brackets_ >;
    _cli_options_map_["--include-prefix"] = 
    &::cli::thunk< options, std::string, &options::include_prefix_,
      &options::include_prefix_specified_ >;
    _cli_options_map_["--guard-prefix"] = 
    &::cli::thunk< options, std::string, &options::guard_prefix_,
      &options::guard_prefix_specified_ >;
    _cli_options_map_["--options-file"] = 
    &::cli::thunk< options, std::string, &options::options_file_,
      &options::options_file_specified_ >;
    _cli_options_map_["--trace"] = 
    &::cli::thunk< options, bool, &options::trace_ >;
    _cli_options_map_["--mysql-engine"] = 
    &::cli::thunk< options, std::string, &options::mysql_engine_,
      &options::mysql_engine_specified_ >;
  }
} _cli_options_map_init_;

void options::
_parse (::cli::scanner& s,
        ::cli::unknown_mode opt_mode,
        ::cli::unknown_mode arg_mode)
{
  bool opt = true;

  while (s.more ())
  {
    const char* o = s.peek ();

    if (std::strcmp (o, "--") == 0)
    {
      s.skip ();
      opt = false;
      continue;
    }

    _cli_options_map::const_iterator i (
      opt ? _cli_options_map_.find (o) : _cli_options_map_.end ());

    if (i != _cli_options_map_.end ())
    {
      (*(i->second)) (*this, s);
    }
    else if (opt && std::strncmp (o, "-", 1) == 0 && o[1] != '\0')
    {
      switch (opt_mode)
      {
        case ::cli::unknown_mode::skip:
        {
          s.skip ();
          continue;
        }
        case ::cli::unknown_mode::stop:
        {
          break;
        }
        case ::cli::unknown_mode::fail:
        {
          throw ::cli::unknown_option (o);
        }
      }

      break;
    }
    else
    {
      switch (arg_mode)
      {
        case ::cli::unknown_mode::skip:
        {
          s.skip ();
          continue;
        }
        case ::cli::unknown_mode::stop:
        {
          break;
        }
        case ::cli::unknown_mode::fail:
        {
          throw ::cli::unknown_argument (o);
        }
      }

      break;
    }
  }
}

