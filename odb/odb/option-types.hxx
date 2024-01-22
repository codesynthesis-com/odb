// file      : odb/option-types.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_OPTION_TYPES_HXX
#define ODB_OPTION_TYPES_HXX

#include <map>
#include <iosfwd>
#include <string>
#include <cassert>

#include <odb/semantics/relational/name.hxx>
#include <odb/semantics/relational/deferrable.hxx>

using semantics::relational::qname;
using semantics::relational::deferrable;

struct cxx_version
{
  enum value
  {
    cxx98,
    cxx11,
    cxx14,
    cxx17,
    cxx20
  };

  cxx_version (value v = value (0)) : v_ (v) {}
  operator value () const {return v_;}

  std::string
  string () const;

private:
  value v_;
};

std::istream&
operator>> (std::istream&, cxx_version&);

//
//
struct database
{
  enum value
  {
    // Keep in alphabetic order.
    //
    common,
    mssql,
    mysql,
    oracle,
    pgsql,
    sqlite
  };

  database (value v = value (0)) : v_ (v) {}
  operator value () const {return v_;}

  std::string
  string () const;

  // Full name (e.g., PostgreSQL).
  //
  std::string
  name () const;

private:
  value v_;
};

std::istream&
operator>> (std::istream&, database&);

std::ostream&
operator<< (std::ostream&, database);

//
//
template <typename V>
struct database_map: std::map<database, V>
{
  typedef std::map<database, V> base_type;

  using base_type::operator[];

  V const&
  operator[] (database const& k) const
  {
    typename base_type::const_iterator i (this->find (k));
    assert (i != this->end ());
    return i->second;
  }
};

//
//
struct multi_database
{
  enum value
  {
    // Keep in alphabetic order.
    //
    dynamic,
    static_,
    disabled // Special value.
  };

  multi_database (value v = disabled) : v_ (v) {}
  operator value () const {return v_;}

  std::string
  string () const;

private:
  value v_;
};

std::istream&
operator>> (std::istream&, multi_database&);

std::ostream&
operator<< (std::ostream&, multi_database);

//
//
struct schema_format
{
  enum value
  {
    // Keep in alphabetic order.
    //
    embedded,
    separate,
    sql
  };

  schema_format (value v = value (0)) : v_ (v) {}
  operator value () const {return v_;}

  std::string
  string () const;

private:
  value v_;
};

std::istream&
operator>> (std::istream&, schema_format&);

std::ostream&
operator<< (std::ostream&, schema_format);

//
//
struct name_case
{
  enum value
  {
    upper,
    lower
  };

  name_case (value v = value (0)) : v_ (v) {}
  operator value () const {return v_;}

private:
  value v_;
};

std::istream&
operator>> (std::istream&, name_case&);

//
//
struct pgsql_version
{
  pgsql_version (unsigned short major, unsigned short minor)
      : major_ (major), minor_ (minor)
  {
  }

  unsigned short
  ver_major () const
  {
    return major_;
  }

  unsigned short
  ver_minor () const
  {
    return minor_;
  }

private:
  unsigned short major_;
  unsigned short minor_;
};

inline bool
operator== (const pgsql_version& x, const pgsql_version& y)
{
  return x.ver_major () == y.ver_major ();
}

inline bool
operator!= (const pgsql_version& x, const pgsql_version& y)
{
  return !(x == y);
}

inline bool
operator< (const pgsql_version& x, const pgsql_version& y)
{
  return x.ver_major () < y.ver_major () ||
    (x.ver_major () == y.ver_major () &&
     x.ver_minor () <  y.ver_minor ());
}

inline bool
operator> (const pgsql_version& x, const pgsql_version& y)
{
  return x.ver_major () > y.ver_major () ||
    (x.ver_major () == y.ver_major () &&
     x.ver_minor () > y.ver_minor ());
}

inline bool
operator<= (const pgsql_version& x, const pgsql_version& y)
{
  return !(x > y);
}

inline bool
operator>= (const pgsql_version& x, const pgsql_version& y)
{
  return !(x < y);
}

std::istream&
operator>> (std::istream&, pgsql_version&);

std::ostream&
operator<< (std::ostream&, pgsql_version);

//
//
struct oracle_version
{
  oracle_version (unsigned short major, unsigned short minor)
      : major_ (major), minor_ (minor)
  {
  }

  unsigned short
  ver_major () const
  {
    return major_;
  }

  unsigned short
  ver_minor () const
  {
    return minor_;
  }

private:
  unsigned short major_;
  unsigned short minor_;
};

inline bool
operator== (const oracle_version& x, const oracle_version& y)
{
  return x.ver_major () == y.ver_major ();
}

inline bool
operator!= (const oracle_version& x, const oracle_version& y)
{
  return !(x == y);
}

inline bool
operator< (const oracle_version& x, const oracle_version& y)
{
  return x.ver_major () < y.ver_major () ||
    (x.ver_major () == y.ver_major () &&
     x.ver_minor () <  y.ver_minor ());
}

inline bool
operator> (const oracle_version& x, const oracle_version& y)
{
  return x.ver_major () > y.ver_major () ||
    (x.ver_major () == y.ver_major () &&
     x.ver_minor () > y.ver_minor ());
}

inline bool
operator<= (const oracle_version& x, const oracle_version& y)
{
  return !(x > y);
}

inline bool
operator>= (const oracle_version& x, const oracle_version& y)
{
  return !(x < y);
}

std::istream&
operator>> (std::istream&, oracle_version&);

std::ostream&
operator<< (std::ostream&, oracle_version);

//
//
struct mssql_version
{
  mssql_version (unsigned short major, unsigned short minor)
      : major_ (major), minor_ (minor)
  {
  }

  unsigned short
  ver_major () const
  {
    return major_;
  }

  unsigned short
  ver_minor () const
  {
    return minor_;
  }

private:
  unsigned short major_;
  unsigned short minor_;
};

inline bool
operator== (const mssql_version& x, const mssql_version& y)
{
  return x.ver_major () == y.ver_major ();
}

inline bool
operator!= (const mssql_version& x, const mssql_version& y)
{
  return !(x == y);
}

inline bool
operator< (const mssql_version& x, const mssql_version& y)
{
  return x.ver_major () < y.ver_major () ||
    (x.ver_major () == y.ver_major () &&
     x.ver_minor () <  y.ver_minor ());
}

inline bool
operator> (const mssql_version& x, const mssql_version& y)
{
  return x.ver_major () > y.ver_major () ||
    (x.ver_major () == y.ver_major () &&
     x.ver_minor () > y.ver_minor ());
}

inline bool
operator<= (const mssql_version& x, const mssql_version& y)
{
  return !(x > y);
}

inline bool
operator>= (const mssql_version& x, const mssql_version& y)
{
  return !(x < y);
}

std::istream&
operator>> (std::istream&, mssql_version&);

std::ostream&
operator<< (std::ostream&, mssql_version);

#endif // ODB_OPTION_TYPES_HXX
