// file      : odb/sql-lexer.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <iostream>

#include <odb/sql-lexer.hxx>

using namespace std;

sql_lexer::
sql_lexer ()
    : loc_ ("C"), buf_ (0, 0, 0)
{
}

sql_lexer::
sql_lexer (std::string const& sql)
    : loc_ ("C"), buf_ (0, 0, 0)
{
  lex (sql);
}

void sql_lexer::
lex (std::string const& sql)
{
  is_.str (sql);
  is_.clear ();

  l_ = c_ = 1;
  buf_ = xchar (0, 0, 0);
  eos_ = false;
  unget_  = false;
}

sql_lexer::xchar sql_lexer::
peek ()
{
  if (unget_)
    return buf_;
  else
  {
    if (eos_)
      return xchar (xchar::traits_type::eof (), l_, c_);
    else
    {
      xchar::int_type i (is_.peek ());

      if (i == xchar::traits_type::eof ())
        eos_ = true;

      return xchar (i, l_, c_);
    }
  }
}

sql_lexer::xchar sql_lexer::
get ()
{
  if (unget_)
  {
    unget_ = false;
    return buf_;
  }
  else
  {
    // When is_.get () returns eof, the failbit is also set (stupid,
    // isn't?) which may trigger an exception. To work around this
    // we will call peek() first and only call get() if it is not
    // eof. But we can only call peek() on eof once; any subsequent
    // calls will spoil the failbit (even more stupid).
    //
    xchar c (peek ());

    if (!is_eos (c))
    {
      is_.get ();

      if (c == '\n')
      {
        l_++;
        c_ = 1;
      }
      else
        c_++;
    }

    return c;
  }
}

void sql_lexer::
unget (xchar c)
{
  // Because iostream::unget cannot work once eos is reached,
  // we have to provide our own implementation.
  //
  buf_ = c;
  unget_ = true;
}

sql_token sql_lexer::
next ()
{
  skip_spaces ();

  xchar c (get ());

  if (is_eos (c))
    return sql_token ();

  switch (c)
  {
  case '\'':
    {
      return string_literal (c);
    }
  case '\"':
    {
      return string_literal (c);
    }
  case '`':
    {
      return string_literal (c);
    }
  case ';':
    {
      return sql_token (sql_token::p_semi);
    }
  case ',':
    {
      return sql_token (sql_token::p_comma);
    }
  case '(':
    {
      return sql_token (sql_token::p_lparen);
    }
  case ')':
    {
      return sql_token (sql_token::p_rparen);
    }
  case '=':
    {
      return sql_token (sql_token::p_eq);
    }
  case '-':
    {
      return int_literal (get (), true);
    }
  case '+':
    {
      return int_literal (get (), false);
    }
  }

  if (is_alpha (c) || c == '_')
  {
    return identifier (c);
  }

  if (is_dec_digit (c))
  {
    return int_literal (c);
  }

  ostringstream msg;
  msg << "unexpected character '" << c << "'";
  throw invalid_input (c.line (), c.column (), msg.str ());
}

void sql_lexer::
skip_spaces ()
{
  for (xchar c (peek ());; c = peek ())
  {
    if (is_eos (c) || !is_space (c))
      break;

    get ();
  }
}

sql_token sql_lexer::
identifier (xchar c)
{
  string lexeme;
  lexeme += c;

  for (c = peek ();
       !is_eos (c) && (is_alnum (c) || c == '_');
       c = peek ())
  {
    get ();
    lexeme += c;
  }

  return sql_token (sql_token::t_identifier, lexeme);
}

sql_token sql_lexer::
int_literal (xchar c, bool neg, size_t /*ml*/, size_t /*mc*/)
{
  //size_t ln (neg ? ml : c.line ()), cl (neg ? mc : c.column ());
  string lexeme;

  if (neg)
    lexeme += '-';

  lexeme += c;

  for (c = peek (); !is_eos (c) && is_dec_digit (c); c = peek ())
  {
    get ();
    lexeme += c;
  }

  return sql_token (sql_token::t_int_lit, lexeme);
}

sql_token sql_lexer::
string_literal (xchar c)
{
  //size_t ln (c.line ()), cl (c.column ());
  char q (c);
  string lexeme;
  lexeme += c;

  while (true)
  {
    xchar c = get ();

    if (is_eos (c))
      throw invalid_input (
        c.line (), c.column (), "unterminated quoted string");

    lexeme += c;

    // In SQL, double-quote is used to encode a single quote inside
    // a string.
    //
    if (c == q)
    {
      if (peek () == q)
        get ();
      else
        break;
    }
  }

  return sql_token (sql_token::t_string_lit, lexeme);
}
