// file      : odb/sql-token.ixx
// license   : GNU GPL v3; see accompanying LICENSE file

inline sql_token::token_type sql_token::
type () const
{
  return type_;
}

inline std::string const& sql_token::
identifier () const
{
  return str_;
}

inline sql_token::punctuation_type sql_token::
punctuation () const
{
  return type_ == t_punctuation ? punctuation_ : p_invalid;
}

inline std::string const& sql_token::
literal () const
{
  return str_;
}

inline sql_token::
sql_token ()
    : type_ (t_eos)
{
}

inline sql_token::
sql_token (punctuation_type p)
    : type_ (t_punctuation), punctuation_ (p)
{
}

inline sql_token::
sql_token (token_type t, std::string const& s)
    : type_ (t), str_ (s)
{
}
