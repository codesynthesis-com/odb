// file      : odb/cxx-lexer.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_CXX_LEXER_HXX
#define ODB_CXX_LEXER_HXX

#include <odb/gcc.hxx>

#include <string>

#include <odb/cxx-token.hxx>

// A C++ keyword. This is an extension to libcpp token types. GCC 4.7.0
// adds this define.
//
#if BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR <= 6
#  define CPP_KEYWORD ((cpp_ttype) (N_TTYPES + 1))
#endif

class cxx_lexer
{
public:
  virtual
  ~cxx_lexer ();

public:
  struct invalid_input {};

  virtual cpp_ttype
  next (std::string& token, tree* node = 0) = 0;

  // Location of the last returned token.
  //
  virtual location_t
  location () const = 0;

public:
  static char const* token_spelling[N_TTYPES + 1];
};


// Adapter to scan a saved token sequence. It returns numbers in the same
// form as they were saved in the token sequence.
//
class cxx_tokens_lexer: public cxx_lexer
{
public:
  void
  start (cxx_tokens const&, location_t start_loc = 0);

  virtual cpp_ttype
  next (std::string& token, tree* node = 0);

  virtual location_t
  location () const;

private:
  cxx_tokens const* tokens_;
  cxx_tokens::const_iterator cur_;
  location_t loc_;
};


// A thin wrapper around the pragma_lex() function that recognizes
// CPP_KEYWORD. It returns numbers as tree nodes.
//
class cxx_pragma_lexer: public cxx_lexer
{
public:
  void
  start ();

  // Start with an already extracted (using the pragma_lex() function)
  // token. This function translates the CPP_NAME to CPP_KEYWORD if
  // necessary and returns the string token. It also uses the passed
  // token and type for subsequent calls to next() so after the last
  // next() call they will contain the information about the last
  // token parsed.
  //
  std::string
  start (tree& token, cpp_ttype& type);

  virtual cpp_ttype
  next (std::string& token, tree* node = 0);

  virtual location_t
  location () const;

private:
  std::string
  translate ();

private:
  tree* token_;
  cpp_ttype* type_;

  tree token_data_;
  cpp_ttype type_data_;
};

// A thin wrapper around cpp_reader for lexing C++ code fragments. It
// returns numbers as string literals.
//
class cxx_string_lexer: public cxx_lexer
{
public:
  cxx_string_lexer ();

  virtual
  ~cxx_string_lexer ();

public:
  void
  start (std::string const&);

  virtual cpp_ttype
  next (std::string& token, tree* node = 0);

  virtual location_t
  location () const;

private:
  std::string data_;
  std::string buf_;
  line_maps line_map_;
  cpp_reader* reader_;
  cpp_callbacks* callbacks_;
  location_t loc_;
};

#endif // ODB_CXX_LEXER_HXX
