// file      : odb/oracle/query.txx
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    // query
    //

    template <database_type_id ID>
    query::
    query (const query_column<bool, ID>& c)
        : binding_ (0, 0)
    {
      // Cannot use IS TRUE here since database type can be a non-
      // integral type.
      //
      append (c.table (), c.column ());
      append ("=");
      append<bool, ID> (val_bind<bool> (true, c.prec (), c.scale ()));
    }

    // query_column
    //
    template <typename T, database_type_id ID>
    query query_column<T, ID>::
    in (const T& v1, const T& v2) const
    {
      query q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_));
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    query query_column<T, ID>::
    in (const T& v1, const T& v2, const T& v3) const
    {
      query q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v3, prec_, scale_));
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    query query_column<T, ID>::
    in (const T& v1, const T& v2, const T& v3, const T& v4) const
    {
      query q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v3, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v4, prec_, scale_));
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    query query_column<T, ID>::
    in (const T& v1, const T& v2, const T& v3, const T& v4, const T& v5) const
    {
      query q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v3, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v4, prec_, scale_));
      q += ",";
      q.append<T, ID> (val_bind<T> (v5, prec_, scale_));
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    template <typename I>
    query query_column<T, ID>::
    in_range (I begin, I end) const
    {
      query q (table_, column_);
      q += "IN (";

      for (I i (begin); i != end; ++i)
      {
        if (i != begin)
          q += ",";

        q.append<T, ID> (val_bind<T> (*i, prec_, scale_));
      }
      q += ")";
      return q;
    }
  }
}
