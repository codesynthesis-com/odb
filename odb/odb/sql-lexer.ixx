// file      : odb/sql-lexer.ixx
// license   : GNU GPL v3; see accompanying LICENSE file

// sql_lexer::xchar
//
inline sql_lexer::xchar::
xchar (int_type v, std::size_t l, std::size_t c)
    : v_ (v), l_ (l), c_ (c)
{
}

inline sql_lexer::xchar::
operator char_type () const
{
  return traits_type::to_char_type (v_);
}

inline sql_lexer::xchar::int_type sql_lexer::xchar::
value () const
{
  return v_;
}

inline std::size_t sql_lexer::xchar::
line () const
{
  return l_;
}

inline std::size_t sql_lexer::xchar::
column () const
{
  return c_;
}

// lexer
//
inline bool sql_lexer::
is_alpha (char c) const
{
  return std::isalpha (c, loc_);
}

inline bool sql_lexer::
is_oct_digit (char c) const
{
  return std::isdigit (c, loc_) && c != '8' && c != '9';
}

inline bool sql_lexer::
is_dec_digit (char c) const
{
  return std::isdigit (c, loc_);
}

inline bool sql_lexer::
is_hex_digit (char c) const
{
  return std::isxdigit (c, loc_);
}

inline bool sql_lexer::
is_alnum (char c) const
{
  return std::isalnum (c, loc_);
}

inline bool sql_lexer::
is_space (char c) const
{
  return std::isspace (c, loc_);
}

inline bool sql_lexer::
is_eos (xchar const& c) const
{
  return  c.value () == xchar::traits_type::eof ();
}

inline char sql_lexer::
to_upper (char c) const
{
  return std::toupper (c, loc_);
}
