// file      : odb/pgsql/query.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace pgsql
  {
    inline native_binding& query_base::
    parameters_binding () const
    {
      return native_binding_;
    }

    template <typename T, database_type_id ID>
    inline void query_base::
    append (val_bind<T> v, const char* conv)
    {
      append (
        std::shared_ptr<query_param> (
          std::make_shared<query_param_impl<T, ID>> (v)),
        conv);
    }

    template <typename T, database_type_id ID>
    inline void query_base::
    append (ref_bind<T> r, const char* conv)
    {
      append (
        std::shared_ptr<query_param> (
          std::make_shared<query_param_impl<T, ID>> (r)),
        conv);
    }
  }
}
