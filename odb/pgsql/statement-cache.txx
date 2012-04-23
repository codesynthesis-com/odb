// file      : odb/pgsql/statement-cache.txx
// copyright : Copyright (c) 2005-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  namespace pgsql
  {
    template <typename T>
    typename object_traits<T>::statements_type& statement_cache::
    find_object ()
    {
      typedef typename object_traits<T>::statements_type statements_type;

      map::iterator i (map_.find (&typeid (T)));

      if (i != map_.end ())
        return static_cast<statements_type&> (*i->second);

      details::shared_ptr<statements_type> p (
        new (details::shared) statements_type (conn_));

      map_.insert (map::value_type (&typeid (T), p));
      return *p;
    }

    template <typename T>
    view_statements<T>& statement_cache::
    find_view ()
    {
      map::iterator i (map_.find (&typeid (T)));

      if (i != map_.end ())
        return static_cast<view_statements<T>&> (*i->second);

      details::shared_ptr<view_statements<T> > p (
        new (details::shared) view_statements<T> (conn_));

      map_.insert (map::value_type (&typeid (T), p));
      return *p;
    }
  }
}
