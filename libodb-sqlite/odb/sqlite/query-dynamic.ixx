// file      : odb/sqlite/query-dynamic.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace sqlite
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
                  const char* table, const char* column, const char* conv)
        : B (table, column, conv)
    {
      native_column_info& ci (qc.native_info[id_sqlite]);
      ci.column = static_cast<query_column_base*> (this);
      ci.param_factory = B::param_factory ();
    }
  }
}
