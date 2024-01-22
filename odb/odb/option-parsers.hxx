// file      : odb/option-parsers.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_OPTION_PARSERS_HXX
#define ODB_OPTION_PARSERS_HXX

#include <vector>
#include <sstream>

#include <odb/option-types.hxx>
#include <odb/options.hxx>

namespace cli
{
  // Return true if there is a database prefix.
  //
  template <typename V>
  bool
  parse_option_value (std::string const& o, std::string const& ov,
                      database& k, V& v)
  {
    bool r (false);
    std::string::size_type p = ov.find (':');

    std::string vstr;
    if (p != std::string::npos)
    {
      std::string kstr (ov, 0, p);

      // See if this prefix resolves to the database name. If not,
      // assume there is no prefix.
      //
      std::istringstream ks (kstr);

      if (ks >> k && ks.eof ())
      {
        r = true;
        vstr.assign (ov, p + 1, std::string::npos);
      }
    }

    if (!r)
      vstr = ov; // Use the whole value.

    if (!vstr.empty ())
    {
      std::istringstream vs (vstr);

      if (!(vs >> v && vs.eof ()))
        throw invalid_value (o, ov);
    }
    else
      v = V ();

    return r;
  }

  // Specialization for std::string.
  //
  bool
  parse_option_value (std::string const&, std::string const& ov,
                      database& k, std::string& v)
  {
    bool r (false);
    std::string::size_type p = ov.find (':');

    if (p != std::string::npos)
    {
      std::string kstr (ov, 0, p);

      // See if this prefix resolves to the database name. If not,
      // assume there is no prefix.
      //
      std::istringstream ks (kstr);

      if (ks >> k && ks.eof ())
      {
        r = true;
        v.assign (ov, p + 1, std::string::npos);
      }
    }

    if (!r)
      v = ov; // Use the whole value.

    return r;
  }

  template <typename V>
  struct parser<database_map<V> >
  {
    typedef database_map<V> map;

    static void
    parse (map& m, bool& xs, scanner& s)
    {
      xs = true;
      std::string o (s.next ());

      if (s.more ())
      {
        database k;
        V v;

        if (parse_option_value (o, s.next (), k, v))
          m[k] = v; // Override any old value.
        else
        {
          // No database prefix is specified which means it applies to
          // all the databases. We also don't want to override database-
          // specific values, so use insert().
          //
          m.insert (typename map::value_type (database::common, v));
          m.insert (typename map::value_type (database::mssql, v));
          m.insert (typename map::value_type (database::mysql, v));
          m.insert (typename map::value_type (database::oracle, v));
          m.insert (typename map::value_type (database::pgsql, v));
          m.insert (typename map::value_type (database::sqlite, v));
        }
      }
      else
        throw missing_value (o);
    }
  };

  template <typename V>
  struct parser<database_map<std::vector<V> > >
  {
    typedef database_map<std::vector<V> > map;

    static void
    parse (map& m, bool& xs, scanner& s)
    {
      xs = true;
      std::string o (s.next ());

      if (s.more ())
      {
        database k;
        V v;

        if (parse_option_value (o, s.next (), k, v))
          m[k].push_back (v);
        else
        {
          // No database prefix is specified which means it applies to
          // all the databases.
          //
          m[database::common].push_back (v);
          m[database::mssql].push_back (v);
          m[database::mysql].push_back (v);
          m[database::oracle].push_back (v);
          m[database::pgsql].push_back (v);
          m[database::sqlite].push_back (v);
        }
      }
      else
        throw missing_value (o);
    }
  };

  template <typename V>
  struct parser<database_map<std::set<V> > >
  {
    typedef database_map<std::set<V> > map;

    static void
    parse (map& m, bool& xs, scanner& s)
    {
      xs = true;
      std::string o (s.next ());

      if (s.more ())
      {
        database k;
        V v;

        if (parse_option_value (o, s.next (), k, v))
          m[k].insert (v);
        else
        {
          // No database prefix is specified which means it applies to
          // all the databases.
          //
          m[database::common].insert (v);
          m[database::mssql].insert (v);
          m[database::mysql].insert (v);
          m[database::oracle].insert (v);
          m[database::pgsql].insert (v);
          m[database::sqlite].insert (v);
        }
      }
      else
        throw missing_value (o);
    }
  };
}

#endif // ODB_OPTION_PARSERS_HXX
