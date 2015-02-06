// file      : odb/oracle/query-dynamic.txx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    template <typename T, database_type_id ID>
    details::shared_ptr<query_param>
    query_param_factory_impl (const void* val,
                              const query_column_base& qc,
                              bool by_ref)
    {
      const T& v (*static_cast<const T*> (val));

      unsigned short p (qc.prec ());
      short s (qc.scale ());

      return details::shared_ptr<query_param> (
        by_ref
        ? new (details::shared) query_param_impl<T, ID> (ref_bind<T> (v, p, s))
        : new (details::shared) query_param_impl<T, ID> (val_bind<T> (v, p, s)));
    }
  }
}
