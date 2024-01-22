// file      : odb/option-types.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <istream>
#include <ostream>
#include <algorithm> // std::lower_bound

#include <odb/option-types.hxx>

using namespace std;

//
// cxx_version
//

static const char* cxx_version_[] =
{
  "c++98",
  "c++11",
  "c++14",
  "c++17",
  "c++20"
};

string cxx_version::
string () const
{
  return cxx_version_[v_];
}

istream&
operator>> (istream& is, cxx_version& v)
{
  string s;
  is >> s;

  if (!is.fail ())
  {
    if (s == "c++98")
      v = cxx_version::cxx98;
    else if (s == "c++11")
      v = cxx_version::cxx11;
    else if (s == "c++14")
      v = cxx_version::cxx14;
    else if (s == "c++17")
      v = cxx_version::cxx17;
    else if (s == "c++20")
      v = cxx_version::cxx20;
    else
      is.setstate (istream::failbit);
  }

  return is;
}

//
// database
//

static const char* database_[] =
{
  "common",
  "mssql",
  "mysql",
  "oracle",
  "pgsql",
  "sqlite"
};

static const char* database_name_[] =
{
  "Common Interface",
  "SQL Server",
  "MySQL",
  "Oracle",
  "PostgreSQL",
  "SQLite"
};

string database::
string () const
{
  return database_[v_];
}

string database::
name () const
{
  return database_name_[v_];
}

istream&
operator>> (istream& is, database& db)
{
  string s;
  is >> s;

  if (!is.fail ())
  {
    const char** e (database_ + sizeof (database_) / sizeof (char*));
    const char** i (lower_bound (database_, e, s));

    if (i != e && *i == s)
      db = database::value (i - database_);
    else
      is.setstate (istream::failbit);
  }

  return is;
}

ostream&
operator<< (ostream& os, database db)
{
  return os << db.string ();
}

//
// multi_database
//

static const char* multi_database_[] =
{
  "dynamic",
  "static",
  "disabled"
};

string multi_database::
string () const
{
  return multi_database_[v_];
}

istream&
operator>> (istream& is, multi_database& db)
{
  string s;
  is >> s;

  if (!is.fail ())
  {
    const char** e (
      multi_database_ + sizeof (multi_database_) / sizeof (char*) - 1);
    const char** i (lower_bound (multi_database_, e, s));

    if (i != e && *i == s)
      db = multi_database::value (i - multi_database_);
    else
      is.setstate (istream::failbit);
  }

  return is;
}

ostream&
operator<< (ostream& os, multi_database db)
{
  return os << db.string ();
}

//
// schema_format
//

static const char* schema_format_[] =
{
  "embedded",
  "separate",
  "sql"
};

string schema_format::
string () const
{
  return schema_format_[v_];
}

istream&
operator>> (istream& is, schema_format& sf)
{
  string s;
  is >> s;

  if (!is.fail ())
  {
    const char** e (schema_format_ + sizeof (schema_format_) / sizeof (char*));
    const char** i (lower_bound (schema_format_, e, s));

    if (i != e && *i == s)
      sf = schema_format::value (i - schema_format_);
    else
      is.setstate (istream::failbit);
  }

  return is;
}

ostream&
operator<< (ostream& os, schema_format sf)
{
  return os << sf.string ();
}

//
// name_case
//

istream&
operator>> (istream& is, name_case& v)
{
  string s;
  is >> s;

  if (!is.fail ())
  {
    if (s == "upper")
      v = name_case::upper;
    else if (s == "lower")
      v = name_case::lower;
    else
      is.setstate (istream::failbit);
  }

  return is;
}

//
// pgsql_version
//

istream&
operator>> (istream& is, pgsql_version& v)
{
  unsigned short major, minor;

  // Extract the major version.
  //
  is >> major;

  if (!is.fail ())
  {
    // Extract the decimal point.
    //
    char p;
    is >> p;

    if (!is.fail () && p == '.')
    {
      // Extract the minor version.
      //
      is >> minor;

      if (!is.fail ())
        v = pgsql_version (major, minor);
    }
    else
      is.setstate (istream::failbit);
  }

  return is;
}

ostream&
operator<< (ostream& os, pgsql_version v)
{
  return os << v.ver_major () << '.' << v.ver_minor ();
}

//
// oracle_version
//

istream&
operator>> (istream& is, oracle_version& v)
{
  unsigned short major, minor;

  // Extract the major version.
  //
  is >> major;

  if (!is.fail ())
  {
    // Extract the decimal point.
    //
    char p;
    is >> p;

    if (!is.fail () && p == '.')
    {
      // Extract the minor version.
      //
      is >> minor;

      if (!is.fail ())
        v = oracle_version (major, minor);
    }
    else
      is.setstate (istream::failbit);
  }

  return is;
}

ostream&
operator<< (ostream& os, oracle_version v)
{
  return os << v.ver_major () << '.' << v.ver_minor ();
}

//
// mssql_version
//

istream&
operator>> (istream& is, mssql_version& v)
{
  unsigned short major, minor;

  // Extract the major version.
  //
  is >> major;

  if (!is.fail ())
  {
    // Extract the decimal point.
    //
    char p;
    is >> p;

    if (!is.fail () && p == '.')
    {
      // Extract the minor version.
      //
      is >> minor;

      if (!is.fail ())
        v = mssql_version (major, minor);
    }
    else
      is.setstate (istream::failbit);
  }

  return is;
}

ostream&
operator<< (ostream& os, mssql_version v)
{
  return os << v.ver_major () << '.' << v.ver_minor ();
}
