// file      : odb/sql-lexer.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SQL_LEXER_HXX
#define ODB_SQL_LEXER_HXX

#include <string>
#include <locale>
#include <cstddef> // std::size_t
#include <sstream>

#include <odb/sql-token.hxx>

class sql_lexer
{
public:
  sql_lexer ();
  sql_lexer (std::string const& sql);

  void lex (std::string const& sql);

  struct invalid_input
  {
    invalid_input (std::size_t l, std::size_t c, std::string const& m)
        : line (l), column (c), message (m)
    {
    }

    std::size_t line;
    std::size_t column;
    std::string const message;
  };

  sql_token
  next ();

protected:
  class xchar
  {
  public:
    typedef std::char_traits<char> traits_type;
    typedef traits_type::int_type int_type;
    typedef traits_type::char_type char_type;

    xchar (int_type v, std::size_t l, std::size_t c);

    operator char_type () const;

    int_type
    value () const;

    std::size_t
    line () const;

    std::size_t
    column () const;

  private:
    int_type v_;
    std::size_t l_;
    std::size_t c_;
  };

  xchar
  peek ();

  xchar
  get ();

  void
  unget (xchar);

protected:
  void
  skip_spaces ();

  sql_token
  identifier (xchar);

  sql_token
  int_literal (xchar,
               bool neg = false,
               std::size_t ml = 0,
               std::size_t mc = 0);

  sql_token
  string_literal (xchar);

protected:
  bool
  is_alpha (char c) const;

  bool
  is_oct_digit (char c) const;

  bool
  is_dec_digit (char c) const;

  bool
  is_hex_digit (char c) const;

  bool
  is_alnum (char c) const;

  bool
  is_space (char c) const;

  bool
  is_eos (xchar const& c) const;

  char
  to_upper (char c) const;

private:
  std::locale loc_;
  std::istringstream is_;
  std::size_t l_;
  std::size_t c_;

  bool eos_;
  bool include_;

  xchar buf_;
  bool unget_;
};

#include <odb/sql-lexer.ixx>

#endif // ODB_SQL_LEXER_HXX
