// file      : odb/qt/smart-ptr/lazy-ptr.txx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

template <class T>
template <class X>
bool QLazySharedPointer<T>::
equal (const QLazySharedPointer<X>& r) const
{
  bool t1 (!p_ == loaded ());
  bool t2 (!r.p_ == r.loaded ());

  // If both are transient, then compare the underlying pointers.
  //
  if (t1 && t2)
    return p_ == r.p_;

  // If one is transient and the other is persistent, then compare
  // the underlying pointers but only if they are non NULL. Note
  // that an unloaded persistent object is always unequal to a
  // transient object.
  //
  if (t1 || t2)
    return p_ == r.p_ && p_;

  // If both objects are persistent, then we compare databases and
  // object ids.
  //
  typedef typename odb::object_traits<T>::object_type object_type1;
  typedef typename odb::object_traits<X>::object_type object_type2;

  return i_.database () == r.i_.database () &&
    objectId<object_type1> () == r.template objectId<object_type2> ();
}

//
// QLazyWeakPointer
//

template <class T>
template <class X>
bool QLazyWeakPointer<T>::
equal (const QLazyWeakPointer<X>& r) const
{
  if (isNull () && r.isNull ())
    return true;

  QLazySharedPointer<T> sp1 (toStrongRef ());
  QLazySharedPointer<T> sp2 (r.toStrongRef ());

  // If either one has expired, they are not equal.
  //
  if (!sp1 || !sp2)
    return false;

  return sp1.equal (sp2);
}

template <class T>
template <class X>
bool QLazyWeakPointer<T>::
equal (const QLazySharedPointer<X>& r) const
{
  if (isNull () && r.isNull ())
    return true;

  QLazySharedPointer<T> sp (toStrongRef ());

  // If the weak pointer has expired, they are not equal.
  //
  if (!sp)
    return false;

  return r.equal (sp);
}

template <class T>
QLazySharedPointer<T> QLazyWeakPointer<T>::
toStrongRef () const
{
  QSharedPointer<T> sp (p_.toStrongRef ());

  if (sp)
  {
    if (database_type* db = i_.database ())
      return QLazySharedPointer<T> (*db, sp);
    else
      return QLazySharedPointer<T> (sp);
  }
  else
  {
    if (i_)
      return QLazySharedPointer<T> (
        *i_.database (), i_.template object_id<T> ());
    else
      return QLazySharedPointer<T> ();
  }
}
