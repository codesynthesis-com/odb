// file      : odb/oracle/query.txx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    //
    // query_base
    //

    template <database_type_id ID>
    query_base::
    query_base (const query_column<bool, ID>& c)
        : binding_ (0, 0)
    {
      // Cannot use IS TRUE here since database type can be a non-
      // integral type.
      //
      append (c.table (), c.column ());
      append ("=");
      append<bool, ID> (val_bind<bool> (true, c.prec (), c.scale ()),
                        c.conversion ());
    }

    //
    // query_column
    //

    // in
    //
    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    in (decayed_type v1, decayed_type v2) const
    {
      query_base q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_), conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    in (decayed_type v1, decayed_type v2, decayed_type v3) const
    {
      query_base q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v3, prec_, scale_), conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    in (decayed_type v1, decayed_type v2, decayed_type v3,
        decayed_type v4) const
    {
      query_base q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v3, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v4, prec_, scale_), conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    in (decayed_type v1, decayed_type v2, decayed_type v3, decayed_type v4,
        decayed_type v5) const
    {
      query_base q (table_, column_);
      q += "IN (";
      q.append<T, ID> (val_bind<T> (v1, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v2, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v3, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v4, prec_, scale_), conversion_);
      q += ",";
      q.append<T, ID> (val_bind<T> (v5, prec_, scale_), conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID>
    template <typename I>
    query_base query_column<T, ID>::
    in_range (I begin, I end) const
    {
      if (begin != end)
      {
        query_base q (table_, column_);
        q += "IN (";

        for (I i (begin); i != end; ++i)
        {
          if (i != begin)
            q += ",";

          q.append<T, ID> (val_bind<T> (*i, prec_, scale_), conversion_);
        }

        q += ")";
        return q;
      }
      else
        return query_base (false);
    }

    // like
    //
    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    like (val_bind<T> p) const
    {
      query_base q (table_, column_);
      q += "LIKE";
      q.append<T, ID> (p, conversion_);
      return q;
    }

    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    like (ref_bind<T> p) const
    {
      query_base q (table_, column_);
      q += "LIKE";
      q.append<T, ID> (p, conversion_);
      return q;
    }

    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    like (val_bind<T> p, decayed_type e) const
    {
      query_base q (table_, column_);
      q += "LIKE";
      q.append<T, ID> (p, conversion_);
      q += "ESCAPE";
      q.append<T, ID> (val_bind<T> (e), conversion_);
      return q;
    }

    template <typename T, database_type_id ID>
    query_base query_column<T, ID>::
    like (ref_bind<T> p, decayed_type e) const
    {
      query_base q (table_, column_);
      q += "LIKE";
      q.append<T, ID> (p, conversion_);
      q += "ESCAPE";
      q.append<T, ID> (val_bind<T> (e), conversion_);
      return q;
    }
  }
}
