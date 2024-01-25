// file      : odb/pgsql/database.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

#include <utility> // move()

#include <odb/pgsql/transaction.hxx>

namespace odb
{
  namespace pgsql
  {
#ifdef ODB_CXX11
    inline database::
    database (database&& db) // Has to be inline.
        : odb::database (std::move (db)),
          user_ (std::move (db.user_)),
          password_ (std::move (db.password_)),
          db_ (std::move (db.db_)),
          host_ (std::move (db.host_)),
          port_ (db.port_),
          socket_ext_ (std::move (db.socket_ext_)),
          extra_conninfo_ (std::move (db.extra_conninfo_)),
          conninfo_ (std::move (db.conninfo_)),
          factory_ (std::move (db.factory_))
    {
      factory_->database (*this); // New database instance.
    }
#endif

    inline connection_ptr database::
    connection ()
    {
      // Go through the virtual connection_() function instead of
      // directly to allow overriding.
      //
      return connection_ptr (
        static_cast<pgsql::connection*> (connection_ ()));
    }

    template <typename T>
    inline typename object_traits<T>::id_type database::
    persist (T& obj)
    {
      return persist_<T, id_pgsql> (obj);
    }

    template <typename T>
    inline typename object_traits<T>::id_type database::
    persist (const T& obj)
    {
      return persist_<const T, id_pgsql> (obj);
    }

    template <typename T>
    inline typename object_traits<T>::id_type database::
    persist (T* p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      return persist_<T, id_pgsql> (pobj);
    }

    template <typename T, template <typename> class P>
    inline typename object_traits<T>::id_type database::
    persist (const P<T>& p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      return persist_<T, id_pgsql> (pobj);
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline typename object_traits<T>::id_type database::
    persist (const P<T, A1>& p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      return persist_<T, id_pgsql> (pobj);
    }

    template <typename T, template <typename> class P>
    inline typename object_traits<T>::id_type database::
    persist (P<T>& p)
    {
      const P<T>& cr (p);
      return persist<T, P> (cr);
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline typename object_traits<T>::id_type database::
    persist (P<T, A1>& p)
    {
      const P<T, A1>& cr (p);
      return persist<T, A1, P> (cr);
    }

    template <typename T>
    inline typename object_traits<T>::id_type database::
    persist (const typename object_traits<T>::pointer_type& pobj)
    {
      return persist_<T, id_pgsql> (pobj);
    }

    template <typename I>
    inline void database::
    persist (I b, I e, bool cont)
    {
      persist_<I, id_pgsql> (b, e, cont);
    }

    template <typename T>
    inline typename object_traits<T>::pointer_type database::
    load (const typename object_traits<T>::id_type& id)
    {
      return load_<T, id_pgsql> (id);
    }

    template <typename T>
    inline void database::
    load (const typename object_traits<T>::id_type& id, T& obj)
    {
      return load_<T, id_pgsql> (id, obj);
    }

    template <typename T>
    inline void database::
    load (T& obj, section& s)
    {
      return load_<T, id_pgsql> (obj, s);
    }

    template <typename T>
    inline typename object_traits<T>::pointer_type database::
    find (const typename object_traits<T>::id_type& id)
    {
      return find_<T, id_pgsql> (id);
    }

    template <typename T>
    inline bool database::
    find (const typename object_traits<T>::id_type& id, T& obj)
    {
      return find_<T, id_pgsql> (id, obj);
    }

    template <typename T>
    inline void database::
    reload (T& obj)
    {
      reload_<T, id_pgsql> (obj);
    }

    template <typename T>
    inline void database::
    reload (T* p)
    {
      reload<T> (*p);
    }

    template <typename T, template <typename> class P>
    inline void database::
    reload (const P<T>& p)
    {
      reload (odb::pointer_traits< P<T> >::get_ref (p));
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline void database::
    reload (const P<T, A1>& p)
    {
      reload (odb::pointer_traits< P<T, A1> >::get_ref (p));
    }

    template <typename T, template <typename> class P>
    inline void database::
    reload (P<T>& p)
    {
      reload (odb::pointer_traits< P<T> >::get_ref (p));
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline void database::
    reload (P<T, A1>& p)
    {
      reload (odb::pointer_traits< P<T, A1> >::get_ref (p));
    }

    template <typename T>
    inline void database::
    reload (const typename object_traits<T>::pointer_type& pobj)
    {
      typedef typename object_traits<T>::pointer_type pointer_type;

      reload (odb::pointer_traits<pointer_type>::get_ref (pobj));
    }

    template <typename T>
    inline void database::
    update (T& obj)
    {
      update_<T, id_pgsql> (obj);
    }

    template <typename T>
    inline void database::
    update (T* p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      update_<T, id_pgsql> (pobj);
    }

    template <typename T, template <typename> class P>
    inline void database::
    update (const P<T>& p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      update_<T, id_pgsql> (pobj);
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline void database::
    update (const P<T, A1>& p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      update_<T, id_pgsql> (pobj);
    }

    template <typename T, template <typename> class P>
    inline void database::
    update (P<T>& p)
    {
      const P<T>& cr (p);
      update<T, P> (cr);
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline void database::
    update (P<T, A1>& p)
    {
      const P<T, A1>& cr (p);
      update<T, A1, P> (cr);
    }

    template <typename T>
    inline void database::
    update (const typename object_traits<T>::pointer_type& pobj)
    {
      update_<T, id_pgsql> (pobj);
    }

    template <typename I>
    inline void database::
    update (I b, I e, bool cont)
    {
      update_<I, id_pgsql> (b, e, cont);
    }

    template <typename T>
    inline void database::
    update (const T& obj, const section& s)
    {
      update_<T, id_pgsql> (obj, s);
    }

    template <typename T>
    inline void database::
    erase (const typename object_traits<T>::id_type& id)
    {
      return erase_<T, id_pgsql> (id);
    }

    template <typename T>
    inline void database::
    erase (T& obj)
    {
      return erase_<T, id_pgsql> (obj);
    }

    template <typename T>
    inline void database::
    erase (T* p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      erase_<T, id_pgsql> (pobj);
    }

    template <typename T, template <typename> class P>
    inline void database::
    erase (const P<T>& p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      erase_<T, id_pgsql> (pobj);
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline void database::
    erase (const P<T, A1>& p)
    {
      typedef typename object_traits<T>::pointer_type object_pointer;

      // The passed pointer should be the same or implicit-convertible
      // to the object pointer. This way we make sure the object pointer
      // does not assume ownership of the passed object.
      //
      const object_pointer& pobj (p);

      erase_<T, id_pgsql> (pobj);
    }

    template <typename T, template <typename> class P>
    inline void database::
    erase (P<T>& p)
    {
      const P<T>& cr (p);
      erase<T, P> (cr);
    }

    template <typename T, typename A1, template <typename, typename> class P>
    inline void database::
    erase (P<T, A1>& p)
    {
      const P<T, A1>& cr (p);
      erase<T, A1, P> (cr);
    }

    template <typename T>
    inline void database::
    erase (const typename object_traits<T>::pointer_type& pobj)
    {
      erase_<T, id_pgsql> (pobj);
    }

    template <typename T, typename I>
    inline void database::
    erase (I idb, I ide, bool cont)
    {
      erase_id_<I, T, id_pgsql> (idb, ide, cont);
    }

    template <typename I>
    inline void database::
    erase (I ob, I oe, bool cont)
    {
      erase_object_<I, id_pgsql> (ob, oe, cont);
    }

    template <typename T>
    inline unsigned long long database::
    erase_query ()
    {
      // T is always object_type.
      //
      return erase_query<T> (pgsql::query_base ());
    }

    template <typename T>
    inline unsigned long long database::
    erase_query (const char* q)
    {
      // T is always object_type.
      //
      return erase_query<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline unsigned long long database::
    erase_query (const std::string& q)
    {
      // T is always object_type.
      //
      return erase_query<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline unsigned long long database::
    erase_query (const pgsql::query_base& q)
    {
      // T is always object_type.
      //
      return object_traits_impl<T, id_pgsql>::erase_query (*this, q);
    }

    template <typename T>
    inline unsigned long long database::
    erase_query (const odb::query_base& q)
    {
      // Translate to native query.
      //
      return erase_query<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline result<T> database::
    query ()
    {
      return query<T> (pgsql::query_base ());
    }

    template <typename T>
    inline result<T> database::
    query (const char* q)
    {
      return query<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline result<T> database::
    query (const std::string& q)
    {
      return query<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline result<T> database::
    query (const pgsql::query_base& q)
    {
      // T is always object_type. We also don't need to check for transaction
      // here; object_traits::query () does this.
      //
      return query_<T, id_pgsql>::call (*this, q);
    }

    template <typename T>
    inline result<T> database::
    query (const odb::query_base& q)
    {
      // Translate to native query.
      //
      return query<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline typename result<T>::pointer_type database::
    query_one ()
    {
      return query_one<T> (pgsql::query_base ());
    }

    template <typename T>
    inline bool database::
    query_one (T& o)
    {
      return query_one<T> (pgsql::query_base (), o);
    }

    template <typename T>
    inline T database::
    query_value ()
    {
      return query_value<T> (pgsql::query_base ());
    }

    template <typename T>
    inline typename result<T>::pointer_type database::
    query_one (const char* q)
    {
      return query_one<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline bool database::
    query_one (const char* q, T& o)
    {
      return query_one<T> (pgsql::query_base (q), o);
    }

    template <typename T>
    inline T database::
    query_value (const char* q)
    {
      return query_value<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline typename result<T>::pointer_type database::
    query_one (const std::string& q)
    {
      return query_one<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline bool database::
    query_one (const std::string& q, T& o)
    {
      return query_one<T> (pgsql::query_base (q), o);
    }

    template <typename T>
    inline T database::
    query_value (const std::string& q)
    {
      return query_value<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline typename result<T>::pointer_type database::
    query_one (const pgsql::query_base& q)
    {
      // T is always object_type. We also don't need to check for transaction
      // here; object_traits::query () does this.
      //
      return query_one_<T, id_pgsql> (q);
    }

    template <typename T>
    inline bool database::
    query_one (const pgsql::query_base& q, T& o)
    {
      // T is always object_type. We also don't need to check for transaction
      // here; object_traits::query () does this.
      //
      return query_one_<T, id_pgsql> (q, o);
    }

    template <typename T>
    inline T database::
    query_value (const pgsql::query_base& q)
    {
      // T is always object_type. We also don't need to check for transaction
      // here; object_traits::query () does this.
      //
      return query_value_<T, id_pgsql> (q);
    }

    template <typename T>
    inline typename result<T>::pointer_type database::
    query_one (const odb::query_base& q)
    {
      // Translate to native query.
      //
      return query_one<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline bool database::
    query_one (const odb::query_base& q, T& o)
    {
      // Translate to native query.
      //
      return query_one<T> (pgsql::query_base (q), o);
    }

    template <typename T>
    inline T database::
    query_value (const odb::query_base& q)
    {
      // Translate to native query.
      //
      return query_value<T> (pgsql::query_base (q));
    }

    template <typename T>
    inline prepared_query<T> database::
    prepare_query (const char* n, const char* q)
    {
      return prepare_query<T> (n, pgsql::query_base (q));
    }

    template <typename T>
    inline prepared_query<T> database::
    prepare_query (const char* n, const std::string& q)
    {
      return prepare_query<T> (n, pgsql::query_base (q));
    }

    template <typename T>
    inline prepared_query<T> database::
    prepare_query (const char* n, const pgsql::query_base& q)
    {
      // Throws if not in transaction.
      //
      pgsql::connection& c (transaction::current ().connection (*this));
      return c.prepare_query<T> (n, q);
    }

    template <typename T>
    inline prepared_query<T> database::
    prepare_query (const char* n, const odb::query_base& q)
    {
      // Translate to native query.
      //
      return prepare_query<T> (n, pgsql::query_base (q));
    }
  }
}
