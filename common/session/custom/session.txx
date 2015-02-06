// file      : common/session/custom/session.txx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

template <typename T>
typename session::cache_position<T> session::
_cache_insert (odb::database&,
               const typename odb::object_traits<T>::id_type& id,
               const typename odb::object_traits<T>::pointer_type& obj)
{
  if (current == 0)
    return cache_position<T> (); // No session, return empty position.

  std::shared_ptr<object_map_base>& pm (current->map_[&typeid (T)]);

  if (!pm)
    pm.reset (new object_map<T>);

  object_map<T>& m (static_cast<object_map<T>&> (*pm));

  typename object_map<T>::value_type vt (id, object_data<T> (obj));
  std::pair<typename object_map<T>::iterator, bool> r (m.insert (vt));

  // We shall never try to re-insert the same object into the cache.
  //
  assert (r.second);

  return cache_position<T> (m, r.first);
}

template <typename T>
typename odb::object_traits<T>::pointer_type session::
_cache_find (odb::database&, const typename odb::object_traits<T>::id_type& id)
{
  typedef typename odb::object_traits<T>::pointer_type pointer_type;

  if (current == 0)
    return pointer_type (); // No session, return NULL pointer.

  type_map::const_iterator ti (current->map_.find (&typeid (T)));

  if (ti == current->map_.end ())
    return pointer_type ();

  const object_map<T>& m (static_cast<const object_map<T>&> (*ti->second));
  typename object_map<T>::const_iterator oi (m.find (id));

  if (oi == m.end ())
    return pointer_type ();

  return oi->second.obj;
}

template <typename T>
void session::
_cache_load (const cache_position<T>& p)
{
  typedef typename odb::object_traits<T>::pointer_type pointer_type;

  if (p.map_ == 0)
    return; // Empty position.

  // Make a copy for change tracking. If our object model had a
  // polymorphic hierarchy, then we would have had to use a
  // virtual function-based mechanism (e.g., clone()) instead of
  // the copy constructor since for a polymorphic hierarchy all
  // the derived objects are stored as pointers to the root object.
  //
  p.pos_->second.orig = pointer_type (new T (*p.pos_->second.obj));
}

template <typename T>
void session::
_cache_update (odb::database&, const T& obj)
{
  typedef odb::object_traits<T> object_traits;
  typedef typename object_traits::pointer_type pointer_type;

  if (current == 0)
    return; // No session.

  // User explicitly updated the object by calling database::update().
  // Change the state to flushed and reset the original copy (we are
  // still tracking changes after the update).
  //
  type_map::iterator ti (current->map_.find (&typeid (T)));

  if (ti == current->map_.end ())
    return; // This object is not in the session.

  object_map<T>& m (static_cast<object_map<T>&> (*ti->second));
  typename object_map<T>::iterator oi (m.find (object_traits::id (obj)));

  if (oi == m.end ())
    return; // This object is not in the session.

  object_data<T>& d (oi->second);
  d.orig = pointer_type (new T (*d.obj));
  d.state = flushed;
}

template <typename T>
void session::
_cache_erase (odb::database&,
              const typename odb::object_traits<T>::id_type& id)
{
  if (current == 0)
    return; // No session.

  type_map::iterator ti (current->map_.find (&typeid (T)));

  if (ti == current->map_.end ())
    return;

  object_map<T>& m (static_cast<object_map<T>&> (*ti->second));
  typename object_map<T>::iterator oi (m.find (id));

  if (oi == m.end ())
    return;

  m.erase (oi);

  if (m.empty ())
    current->map_.erase (ti);
}

template <typename T>
bool session::object_map<T>::
flush (odb::database& db)
{
  bool r (false);
  for (typename object_map<T>::iterator i (this->begin ()), e (this->end ());
       i != e; ++i)
  {
    object_data<T>& d (i->second);

    if (d.state == changed || d.obj->changed (*d.orig))
      db.update (d.obj); // State changed by the update() notification.

    r = r || d.state == flushed;
  }

  return r;
}

template <typename T>
void session::object_map<T>::
mark (unsigned short event)
{
  for (typename object_map<T>::iterator i (this->begin ()), e (this->end ());
       i != e; ++i)
  {
    object_data<T>& d (i->second);

    if (d.state == flushed)
      d.state = event == odb::transaction::event_commit ? tracking : changed;
  }
}
