// file      : odb/sql-token.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <ostream>

#include <odb/sql-token.hxx>

using namespace std;

static char punctuation_literals[] = {';', ',', '(', ')', '='};

string sql_token::
string () const
{
  switch (type ())
  {
  case sql_token::t_eos:
    {
      return "<end-of-stream>";
    }
  case sql_token::t_identifier:
    {
      return identifier ();
    }
  case sql_token::t_punctuation:
    {
      return std::string (1, punctuation_literals[punctuation ()]);
    }
  case sql_token::t_string_lit:
  case sql_token::t_int_lit:
  case sql_token::t_float_lit:
    {
      return literal ();
    }
  }

  return "";
}

ostream&
operator<< (ostream& os, sql_token const& t)
{
  return os << t.string ();
}
