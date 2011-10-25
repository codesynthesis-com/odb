// file      : odb/oracle/query.ixx
// author    : Constantin Michael <constantin@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : ODB NCUEL; see accompanying LICENSE file

namespace odb
{
  namespace oracle
  {
    template <typename T, database_type_id ID>
    inline void query::
    append (val_bind<T> v)
    {
      add (
        details::shared_ptr<query_param> (
          new (details::shared) query_param_impl<T, ID> (v)));
    }

    template <typename T, database_type_id ID>
    inline void query::
    append (ref_bind<T> r)
    {
      add (
        details::shared_ptr<query_param> (
          new (details::shared) query_param_impl<T, ID> (r)));
    }
  }
}
