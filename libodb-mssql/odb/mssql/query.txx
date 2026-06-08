// file      : odb/mssql/query.txx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace mssql
  {
    //
    // query_base
    //

    template <database_type_id ID, typename B>
    query_base::
    query_base (const query_column<bool, ID, B>& c)
        : binding_ (0, 0)
    {
      // Cannot use IS TRUE here since database type can be a non-
      // integral type.
      //
      append (c.table (), c.column ());
      append ("=");
      B::append (*this,
                 val_bind<bool> (true, c.prec (), c.scale ()),
                 c.conversion ());
    }

    //
    // query_column
    //

    // in
    //
    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    in (decayed_type v1, decayed_type v2) const
    {
      query_base q (this->table_, this->column_);
      q += "IN (";
      B::append (q,
                 val_bind<T> (v1, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v2, this->prec_, this->scale_),
                 this->conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    in (decayed_type v1, decayed_type v2, decayed_type v3) const
    {
      query_base q (this->table_, this->column_);
      q += "IN (";
      B::append (q,
                 val_bind<T> (v1, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v2, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v3, this->prec_, this->scale_),
                 this->conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    in (decayed_type v1, decayed_type v2, decayed_type v3,
        decayed_type v4) const
    {
      query_base q (this->table_, this->column_);
      q += "IN (";
      B::append (q,
                 val_bind<T> (v1, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v2, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v3, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v4, this->prec_, this->scale_),
                 this->conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    in (decayed_type v1, decayed_type v2, decayed_type v3, decayed_type v4,
        decayed_type v5) const
    {
      query_base q (this->table_, this->column_);
      q += "IN (";
      B::append (q,
                 val_bind<T> (v1, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v2, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v3, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v4, this->prec_, this->scale_),
                 this->conversion_);
      q += ",";
      B::append (q,
                 val_bind<T> (v5, this->prec_, this->scale_),
                 this->conversion_);
      q += ")";
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    template <typename I>
    query_base query_column<T, ID, B>::
    in_range (I begin, I end) const
    {
      if (begin != end)
      {
        query_base q (this->table_, this->column_);
        q += "IN (";

        for (I i (begin); i != end; ++i)
        {
          if (i != begin)
            q += ",";

          B::append (q,
                     val_bind<T> (*i, this->prec_, this->scale_),
                     this->conversion_);
        }

        q += ")";
        return q;
      }
      else
        return query_base (false);
    }

    // like
    //
    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    like (val_bind<T> p) const
    {
      query_base q (this->table_, this->column_);
      q += "LIKE";
      B::append (q, p, this->conversion_);
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    like (ref_bind<T> p) const
    {
      query_base q (this->table_, this->column_);
      q += "LIKE";
      B::append (q, p, this->conversion_);
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    like (val_bind<T> p, decayed_type e) const
    {
      query_base q (this->table_, this->column_);
      q += "LIKE";
      B::append (q, p, this->conversion_);
      q += "ESCAPE";
      B::append (q, val_bind<T> (e), this->conversion_);
      return q;
    }

    template <typename T, database_type_id ID, typename B>
    query_base query_column<T, ID, B>::
    like (ref_bind<T> p, decayed_type e) const
    {
      query_base q (this->table_, this->column_);
      q += "LIKE";
      B::append (q, p, this->conversion_);
      q += "ESCAPE";
      B::append (q, val_bind<T> (e), this->conversion_);
      return q;
    }
  }
}
