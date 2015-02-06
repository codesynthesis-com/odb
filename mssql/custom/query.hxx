// file      : mssql/custom/query.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef QUERY_HXX
#define QUERY_HXX

#include <string>

#include <odb/mssql/query.hxx>

#include "test.hxx" // point

namespace odb
{
  namespace mssql
  {
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
    template <>
    struct query_column<point, id_string>
    {
    private:
      const char* table_;
      const char* column_;
      const char* conversion_;

      unsigned short prec_;
      unsigned short scale_;

      std::string x_column_;
      std::string y_column_;

      // Sub-columns for individual members.
      //
    public:
      query_column<double, id_float8> x, y;

      // is_null, is_not_null
      //
    public:
      query_base
      is_null () const
      {
        query_base q (table_, column_);
        q += "IS NULL";
        return q;
      }

      query_base
      is_not_null () const
      {
        query_base q (table_, column_);
        q += "IS NOT NULL";
        return q;
      }

      // =
      //
    public:
      query_base
      equal (const point& v) const
      {
        return equal (val_bind<point> (v));
      }

      query_base
      equal (val_bind<point> v) const
      {
        query_base q (table_, column_);
        q += ".STEquals(";
        q.append<point, id_string> (v, conversion_);
        q += ") = 1";
        return q;
      }

      query_base
      equal (ref_bind<point> r) const
      {
        query_base q (table_, column_);
        q += ".STEquals(";
        q.append<point, id_string> (r, conversion_);
        q += ") = 1";
        return q;
      }

      friend query_base
      operator== (const query_column& c, const point& v)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (const point& v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (const query_column& c, val_bind<point> v)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (val_bind<point> v, const query_column& c)
      {
        return c.equal (v);
      }

      friend query_base
      operator== (const query_column& c, ref_bind<point> r)
      {
        return c.equal (r);
      }

      friend query_base
      operator== (ref_bind<point> r, const query_column& c)
      {
        return c.equal (r);
      }

      // Column comparison.
      //
    public:
      query_base
      operator== (const query_column<point, id_string>& c) const
      {
        query_base q (table_, column_);
        q += ".STEquals(";
        q.append (c.table (), c.column ());
        q += ") = 1";
        return q;
      }

    public:
      query_column (const char* table,
                    const char* column,
                    const char* conv,
                    unsigned short prec = 0,
                    unsigned short scale = 0xFFFF)
          : table_ (table), column_ (column), conversion_ (conv),
            prec_ (prec), scale_ (scale),
            x_column_ (std::string (column) + ".STX"),
            y_column_ (std::string (column) + ".STY"),
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

            unsigned short
      prec () const
      {
        return prec_;
      }

      unsigned short
      scale () const
      {
        return scale_;
      }
    };
#endif // SQL Server > 2005
  }
}

#endif // QUERY_HXX
