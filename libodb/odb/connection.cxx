// file      : odb/connection.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/database.hxx>
#include <odb/connection.hxx>
#include <odb/result.hxx>
#include <odb/prepared-query.hxx>
#include <odb/exceptions.hxx> // prepared_*

using namespace std;

namespace odb
{
  // connection
  //
  connection::
  ~connection ()
  {
    assert (uncached_prepared_queries_ == nullptr);
    assert (prepared_map_.empty ());
  }

  void connection::
  clear_prepared_map ()
  {
    for (prepared_map_type::iterator i (prepared_map_.begin ()),
           e (prepared_map_.end ()); i != e; ++i)
    {
      if (i->second.params != 0)
        i->second.params_deleter (i->second.params);
    }

    prepared_map_.clear ();
  }

  void connection::
  recycle ()
  {
    while (uncached_prepared_queries_ != nullptr)
    {
      uncached_prepared_queries_->stmt.reset ();
      uncached_prepared_queries_->list_remove ();
    }
  }

  void connection::
  invalidate_results ()
  {
    while (results_ != nullptr)
    {
      results_->invalidate ();
      results_->list_remove ();
    }
  }

  void connection::
  cache_query_ (std::shared_ptr<prepared_query_impl> pq,
                const type_info& ti,
                void* params,
                const type_info* params_info,
                void (*params_deleter) (void*))
  {
    pair<prepared_map_type::iterator, bool> r (
      prepared_map_.insert (
        prepared_map_type::value_type (pq->name, prepared_entry_type ())));

    if (!r.second)
      throw prepared_already_cached (pq->name);

    prepared_entry_type& e (r.first->second);

    // Mark this prepared query as cached and remove it from the invalidation
    // list (prepared_query instances now reference this cached impl object).
    //
    pq->cached = true;
    pq->list_remove ();

    e.prep_query = std::move (pq);
    e.type_info = &ti;
    e.params = params;
    e.params_info = params_info;
    e.params_deleter = params_deleter;
  }

  std::shared_ptr<prepared_query_impl> connection::
  lookup_query_ (const char* name,
                 const type_info& ti,
                 void** params,
                 const type_info* params_info) const
  {
    prepared_map_type::const_iterator i (prepared_map_.find (name));

    if (i == prepared_map_.end ())
    {
      // Use a factory, if there is one.
      //
      if (factory_.database ().call_query_factory (
            name, const_cast<connection&> (*this)))
        i = prepared_map_.find (name);
    }

    if (i == prepared_map_.end ())
      return 0;

    // Make sure the types match.
    //
    if (*i->second.type_info != ti)
      throw prepared_type_mismatch (name);

    if (params != 0)
    {
      if (*i->second.params_info != *params_info)
        throw prepared_type_mismatch (name);

      *params = i->second.params;
    }

    return i->second.prep_query;
  }
}
