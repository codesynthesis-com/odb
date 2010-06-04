// This code was generated by CLI, a command line interface
// compiler for C++.
//

namespace cli
{
  // unknown_mode
  //
  inline unknown_mode::
  unknown_mode (value v)
  : v_ (v)
  {
  }

  // exception
  //
  inline std::ostream&
  operator<< (std::ostream& os, const exception& e)
  {
    e.print (os);
    return os;
  }

  // unknown_option
  //
  inline unknown_option::
  unknown_option (const std::string& option)
  : option_ (option)
  {
  }

  inline const std::string& unknown_option::
  option () const
  {
    return option_;
  }

  // unknown_argument
  //
  inline unknown_argument::
  unknown_argument (const std::string& argument)
  : argument_ (argument)
  {
  }

  inline const std::string& unknown_argument::
  argument () const
  {
    return argument_;
  }

  // missing_value
  //
  inline missing_value::
  missing_value (const std::string& option)
  : option_ (option)
  {
  }

  inline const std::string& missing_value::
  option () const
  {
    return option_;
  }

  // invalid_value
  //
  inline invalid_value::
  invalid_value (const std::string& option,
                 const std::string& value)
  : option_ (option),  value_ (value)
  {
  }

  inline const std::string& invalid_value::
  option () const
  {
    return option_;
  }

  inline const std::string& invalid_value::
  value () const
  {
    return value_;
  }

  // file_io_failure
  //
  inline file_io_failure::
  file_io_failure (const std::string& file)
  : file_ (file)
  {
  }

  inline const std::string& file_io_failure::
  file () const
  {
    return file_;
  }

  // unmatched_quote
  //
  inline unmatched_quote::
  unmatched_quote (const std::string& argument)
  : argument_ (argument)
  {
  }

  inline const std::string& unmatched_quote::
  argument () const
  {
    return argument_;
  }

  // argv_scanner
  //
  inline argv_scanner::
  argv_scanner (int& argc, char** argv, bool erase)
  : i_ (1), argc_ (argc), argv_ (argv), erase_ (erase)
  {
  }

  inline argv_scanner::
  argv_scanner (int start, int& argc, char** argv, bool erase)
  : i_ (start), argc_ (argc), argv_ (argv), erase_ (erase)
  {
  }

  inline int argv_scanner::
  end () const
  {
    return i_;
  }

  // argv_file_scanner
  //
  inline argv_file_scanner::
  argv_file_scanner (int& argc,
                     char** argv,
                     const std::string& option,
                     bool erase)
  : argv_scanner (argc, argv, erase),
    option_ (option),
    skip_ (false)
  {
  }

  inline argv_file_scanner::
  argv_file_scanner (int start,
                     int& argc,
                     char** argv,
                     const std::string& option,
                     bool erase)
  : argv_scanner (start, argc, argv, erase),
    option_ (option),
    skip_ (false)
  {
  }
}

// options
//

inline const bool& options::
help () const
{
  return this->help_;
}

inline const bool& options::
version () const
{
  return this->version_;
}

inline const ::database& options::
database () const
{
  return this->database_;
}

inline bool options::
database_specified () const
{
  return this->database_specified_;
}

inline const bool& options::
generate_schema () const
{
  return this->generate_schema_;
}

inline const std::string& options::
output_dir () const
{
  return this->output_dir_;
}

inline bool options::
output_dir_specified () const
{
  return this->output_dir_specified_;
}

inline const std::string& options::
odb_file_suffix () const
{
  return this->odb_file_suffix_;
}

inline bool options::
odb_file_suffix_specified () const
{
  return this->odb_file_suffix_specified_;
}

inline const std::string& options::
hxx_suffix () const
{
  return this->hxx_suffix_;
}

inline bool options::
hxx_suffix_specified () const
{
  return this->hxx_suffix_specified_;
}

inline const std::string& options::
ixx_suffix () const
{
  return this->ixx_suffix_;
}

inline bool options::
ixx_suffix_specified () const
{
  return this->ixx_suffix_specified_;
}

inline const std::string& options::
cxx_suffix () const
{
  return this->cxx_suffix_;
}

inline bool options::
cxx_suffix_specified () const
{
  return this->cxx_suffix_specified_;
}

inline const std::string& options::
sql_suffix () const
{
  return this->sql_suffix_;
}

inline bool options::
sql_suffix_specified () const
{
  return this->sql_suffix_specified_;
}

inline const bool& options::
include_with_brackets () const
{
  return this->include_with_brackets_;
}

inline const std::string& options::
include_prefix () const
{
  return this->include_prefix_;
}

inline bool options::
include_prefix_specified () const
{
  return this->include_prefix_specified_;
}

inline const std::string& options::
guard_prefix () const
{
  return this->guard_prefix_;
}

inline bool options::
guard_prefix_specified () const
{
  return this->guard_prefix_specified_;
}

inline const std::string& options::
options_file () const
{
  return this->options_file_;
}

inline bool options::
options_file_specified () const
{
  return this->options_file_specified_;
}

inline const bool& options::
trace () const
{
  return this->trace_;
}

inline const std::string& options::
mysql_engine () const
{
  return this->mysql_engine_;
}

inline bool options::
mysql_engine_specified () const
{
  return this->mysql_engine_specified_;
}

