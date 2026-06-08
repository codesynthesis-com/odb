// file      : odb/oracle/query-dynamic.ixx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    //
    //
    template <typename T, database_type_id ID>
    inline void* default_query_column_base<T, ID>::
    param_factory ()
    {
      // For some reason GCC needs this statically-typed pointer in
      // order to instantiate the functions.
      //
      query_param_factory f (&query_param_factory_impl<T, ID>);
      return reinterpret_cast<void*> (f);
    }

    //
    //
    template <typename T, database_type_id ID, typename B>
    template <typename B2>
    inline query_column<T, ID, B>::
    query_column (odb::query_column<T, B2>& qc,
                  const char* table,
                  const char* column,
                  const char* conv,
                  unsigned short prec,
                  short scale)
        : B (table, column, conv, prec, scale)
    {
      native_column_info& ci (qc.native_info[id_oracle]);
      ci.column = static_cast<query_column_base*> (this);
      ci.param_factory = B::param_factory ();
    }

    template <typename T>
    template <typename B2>
    inline query_column<T, id_blob>::
    query_column (odb::query_column<T, B2>& qc,
                  const char* table, const char* column, const char*)
        : lob_query_column (table, column)
    {
      native_column_info& ci (qc.native_info[id_oracle]);
      ci.column = static_cast<query_column_base*> (this);

      // In Oracle LOBs cannot be compared.
      //
      ci.param_factory = 0;
    }

    template <typename T>
    template <typename B2>
    inline query_column<T, id_clob>::
    query_column (odb::query_column<T, B2>& qc,
                  const char* table, const char* column, const char*)
        : lob_query_column (table, column)
    {
      native_column_info& ci (qc.native_info[id_oracle]);
      ci.column = static_cast<query_column_base*> (this);

      // In Oracle LOBs cannot be compared.
      //
      ci.param_factory = 0;
    }

    template <typename T>
    template <typename B2>
    inline query_column<T, id_nclob>::
    query_column (odb::query_column<T, B2>& qc,
                  const char* table, const char* column, const char*)
        : lob_query_column (table, column)
    {
      native_column_info& ci (qc.native_info[id_oracle]);
      ci.column = static_cast<query_column_base*> (this);

      // In Oracle LOBs cannot be compared.
      //
      ci.param_factory = 0;
    }
  }
}
