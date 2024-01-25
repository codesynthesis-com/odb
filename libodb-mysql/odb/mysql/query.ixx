// file      : odb/mysql/query.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace mysql
  {
    inline binding& query_base::
    parameters_binding () const
    {
      return binding_;
    }

    template <typename T, database_type_id ID>
    inline void query_base::
    append (val_bind<T> v, const char* conv)
    {
      append (
        details::shared_ptr<query_param> (
          new (details::shared) query_param_impl<T, ID> (v)),
        conv);
    }

    template <typename T, database_type_id ID>
    inline void query_base::
    append (ref_bind<T> r, const char* conv)
    {
      append (
        details::shared_ptr<query_param> (
          new (details::shared) query_param_impl<T, ID> (r)),
        conv);
    }
  }
}
