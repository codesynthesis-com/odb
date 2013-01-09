// file      : common/session/custom/session.txx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

template <typename T>
typename session::position<T> session::
insert (odb::database&,
        const typename odb::object_traits<T>::id_type& id,
        const typename odb::object_traits<T>::pointer_type& obj)
{
  typedef odb::object_traits<T> object_traits;

  std::shared_ptr<object_map_base>& pm (map_[&typeid (T)]);

  if (!pm)
    pm.reset (new object_map<T>);

  object_map<T>& m (static_cast<object_map<T>&> (*pm));

  typename object_map<T>::value_type vt (id, object_state<T> (obj));
  std::pair<typename object_map<T>::iterator, bool> r (m.insert (vt));

  // We shall never try to re-insert the same object into the cache.
  //
  assert (r.second);

  return position<T> (m, r.first);
}

template <typename T>
void session::
initialize (const position<T>& p)
{
  typedef typename odb::object_traits<T>::pointer_type pointer_type;

  // Make a copy for change tracking. If our object model had a
  // polymorphic hierarchy, then we would have had to use a
  // virtual function-based mechanism (e.g., clone()) instead of
  // the copy constructor since for a polymorphic hierarchy all
  // the derived objects are stored as pointers to the root object.
  //
  p.pos_->second.orig = pointer_type (new T (*p.pos_->second.obj));
}

template <typename T>
typename odb::object_traits<T>::pointer_type session::
find (odb::database&, const typename odb::object_traits<T>::id_type& id) const
{
  typedef typename odb::object_traits<T>::pointer_type pointer_type;

  type_map::const_iterator ti (map_.find (&typeid (T)));

  if (ti == map_.end ())
    return pointer_type ();

  const object_map<T>& m (static_cast<const object_map<T>&> (*ti->second));
  typename object_map<T>::const_iterator oi (m.find (id));

  if (oi == m.end ())
    return pointer_type ();

  return oi->second.obj;
}

template <typename T>
void session::
erase (odb::database&, const typename odb::object_traits<T>::id_type& id)
{
  type_map::iterator ti (map_.find (&typeid (T)));

  if (ti == map_.end ())
    return;

  object_map<T>& m (static_cast<object_map<T>&> (*ti->second));
  typename object_map<T>::iterator oi (m.find (id));

  if (oi == m.end ())
    return;

  m.erase (oi);

  if (m.empty ())
    map_.erase (ti);
}

template <typename T>
void session::object_map<T>::
flush (odb::database& db)
{
  for (typename object_map<T>::iterator i (this->begin ()), e (this->end ());
       i != e; ++i)
  {
    const T& obj (*i->second.obj);

    if (obj.changed (*i->second.orig))
    {
      db.update (obj);
      i->second.flushed_ = true;
    }
  }
}

template <typename T>
void session::object_map<T>::
mark ()
{
  for (typename object_map<T>::iterator i (this->begin ()), e (this->end ());
       i != e; ++i)
  {
    if (i->second.flushed_)
    {
      i->second.orig.reset (new T (*i->second.obj));
      i->second.flushed_ = false;
    }
  }
}
