// file      : pgsql/custom/query.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef QUERY_HXX
#define QUERY_HXX

#include <string>

#include <odb/pgsql/query.hxx>

#include "test.hxx" // point

namespace odb
{
  namespace pgsql
  {
    template <>
    struct query_column<point, id_string>
    {
    private:
      const char* table_;
      const char* column_;
      const char* conversion_;

      std::string x_column_;
      std::string y_column_;

      // Sub-columns for individual members.
      //
    public:
      query_column<double, id_double> x, y;

      // is_null, is_not_null
      //
    public:
      query
      is_null () const
      {
        query q (table_, column_);
        q += "IS NULL";
        return q;
      }

      query
      is_not_null () const
      {
        query q (table_, column_);
        q += "IS NOT NULL";
        return q;
      }

      // =
      //
    public:
      query
      equal (const point& v) const
      {
        return equal (val_bind<point> (v));
      }

      query
      equal (val_bind<point> v) const
      {
        query q (table_, column_);
        q += "~=";
        q.append<point, id_string> (v, conversion_);
        return q;
      }

      query
      equal (ref_bind<point> r) const
      {
        query q (table_, column_);
        q += "~=";
        q.append<point, id_string> (r, conversion_);
        return q;
      }

      friend query
      operator== (const query_column& c, const point& v)
      {
        return c.equal (v);
      }

      friend query
      operator== (const point& v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query
      operator== (const query_column& c, val_bind<point> v)
      {
        return c.equal (v);
      }

      friend query
      operator== (val_bind<point> v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query
      operator== (const query_column& c, ref_bind<point> r)
      {
        return c.equal (r);
      }

      friend query
      operator== (ref_bind<point> r, const query_column& c)
      {
        return c.equal (r);
      }

      // Column comparison.
      //
    public:
      query
      operator== (const query_column<point, id_string>& c) const
      {
        query q (table_, column_);
        q += "~=";
        q.append (c.table (), c.column ());
        return q;
      }

    public:
      query_column (const char* table, const char* column, const char* conv)
          : table_ (table), column_ (column), conversion_ (conv),
            x_column_ (std::string (column) + "[0]"),
            y_column_ (std::string (column) + "[1]"),
            x (table, x_column_.c_str (), 0),
            y (table, y_column_.c_str (), 0)
      {
      }

      const char*
      table () const
      {
        return table_;
      }

      const char*
      column () const
      {
        return column_;
      }

      const char*
      conversion () const
      {
        return conversion_;
      }
    };
  }
}

#endif // QUERY_HXX
