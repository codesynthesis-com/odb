// file      : odb/mssql/query-dynamic.txx
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace mssql
  {
    template <typename T, database_type_id ID>
    std::shared_ptr<query_param>
    query_param_factory_impl (const void* val,
                              const query_column_base& qc,
                              bool by_ref)
    {
      const T& v (*static_cast<const T*> (val));

      unsigned short p (qc.prec ());
      unsigned short s (qc.scale ());

      return std::shared_ptr<query_param> (
        by_ref
        ? std::make_shared<query_param_impl<T, ID>> (ref_bind<T> (v, p, s))
        : std::make_shared<query_param_impl<T, ID>> (val_bind<T> (v, p, s)));
    }
  }
}
