// file      : odb/qt/smart-ptr/lazy-ptr.ixx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

//
// QLazySharedPointer definition.
//

template <class T>
inline QLazySharedPointer<T>::
QLazySharedPointer () {}

template <class T>
inline QLazySharedPointer<T>::
QLazySharedPointer (T* p): p_ (p) {}

template <class T>
template <class Deleter>
inline QLazySharedPointer<T>::
QLazySharedPointer (T* p, Deleter d): p_ (p, d) {}

template <class T>
inline QLazySharedPointer<T>::
QLazySharedPointer (const QLazySharedPointer& r): p_ (r.p_), i_ (r.i_) {}

template <class T>
template <class X>
inline QLazySharedPointer<T>::
QLazySharedPointer (const QLazySharedPointer<X>& r): p_ (r.p_), i_ (r.i_) {}

template <class T>
template <class X>
inline QLazySharedPointer<T>::
QLazySharedPointer (const QLazyWeakPointer<X>& r): p_ (r.p_), i_ (r.i_) {}

template <class T>
inline QLazySharedPointer<T>::
~QLazySharedPointer () {}

template <class T>
inline QLazySharedPointer<T>& QLazySharedPointer<T>::
operator= (const QLazySharedPointer& r)
{
  p_ = r.p_;
  i_ = r.i_;
  return *this;
}

template <class T>
template <class X>
inline QLazySharedPointer<T>& QLazySharedPointer<T>::
operator= (const QLazySharedPointer<X>& r)
{
  p_ = r.p_;
  i_ = r.i_;
  return *this;
}

template <class T>
template <class X>
inline QLazySharedPointer<T>& QLazySharedPointer<T>::
operator= (const QLazyWeakPointer<X>& r)
{
  p_ = r.p_;
  i_ = r.i_;
  return *this;
}

template <class T>
inline bool QLazySharedPointer<T>::
operator! () const
{
  return isNull ();
}

template <class T>
inline T& QLazySharedPointer<T>::
operator* () const
{
  return *p_;
}

template <class T>
inline T* QLazySharedPointer<T>::
operator-> () const
{
  return p_.operator-> ();
}

template <class T>
inline void QLazySharedPointer<T>::
swap (QLazySharedPointer& x)
{
  p_.swap (x.p_);
  i_.swap (x.i_);
}

template <class T>
inline bool QLazySharedPointer<T>::
isNull () const
{
  return !(p_ || i_);
}

template <class T>
inline T* QLazySharedPointer<T>::
data () const
{
  return p_.data ();
}

template <class T>
inline QLazyWeakPointer<T> QLazySharedPointer<T>::
toWeakRef () const
{
  return QLazyWeakPointer<T> (*this);
}

template <class T>
inline void QLazySharedPointer<T>::
clear ()
{
  p_.clear ();
  i_.reset ();
}

template <class T>
template <class X>
inline QLazySharedPointer<X> QLazySharedPointer<T>::
staticCast () const
{
  QLazySharedPointer c (p_.template staticCast<X> ());
  c.i_ = i_;
  return c;
}

template <class T>
template <class X>
inline QLazySharedPointer<X> QLazySharedPointer<T>::
dynamicCast () const
{
  QLazySharedPointer<X> c (p_.template dynamicCast<X> ());

  if (c)
    c.i_ = i_;

  return c;
}

template <class T>
template <class X>
inline QLazySharedPointer<X> QLazySharedPointer<T>::
constCast () const
{
  QLazySharedPointer<X> c (p_.template constCast<X> ());
  c.i_ = i_;
  return c;
}

template <class T>
template <class Y>
inline QLazySharedPointer<T>::
QLazySharedPointer (const QSharedPointer<Y>& r): p_ (r) {}

template <class T>
template <class Y>
inline QLazySharedPointer<T>::
QLazySharedPointer (const QWeakPointer<Y>& r): p_ (r) {}

template <class T>
template <class X>
inline QLazySharedPointer<T>& QLazySharedPointer<T>::
operator= (const QSharedPointer<X>& r)
{
  p_ = r;
  i_.reset ();
  return *this;
}

template <class T>
template <class X>
inline QLazySharedPointer<T>& QLazySharedPointer<T>::
operator= (const QWeakPointer<X>& r)
{
  p_ = r;
  i_.reset ();
  return *this;
}

template <class T>
inline bool QLazySharedPointer<T>::
loaded () const
{
  bool i (i_);
  return !p_ != i; // !p_ XOR i_
}

template <class T>
inline QSharedPointer<T> QLazySharedPointer<T>::
load () const
{
  if (!p_ && i_)
    p_ = i_.template load<T> (true); // Reset id.

  return p_;
}

template <class T>
inline void QLazySharedPointer<T>::
unload () const
{
  typedef typename odb::object_traits<T>::object_type object_type;

  if (p_)
  {
    if (i_.database () != 0)
      i_.reset_id (odb::object_traits<object_type>::id (*p_));

    p_.clear ();
  }
}

template <class T>
inline QSharedPointer<T> QLazySharedPointer<T>::
getEager () const
{
  return p_;
}

template <class T>
template <class DB, class ID>
inline QLazySharedPointer<T>::
QLazySharedPointer (DB& db, const ID& id): i_ (db, id) {}

template <class T>
template <class DB>
inline QLazySharedPointer<T>::
QLazySharedPointer (DB& db, T* p)
    : p_ (p)
{
  if (p_)
    i_.reset_db (db);
}

template <class T>
template <class DB, class Deleter>
inline QLazySharedPointer<T>::
QLazySharedPointer (DB& db, T* p, Deleter d)
    : p_ (p, d)
{
  if (p_)
    i_.reset_db (db);
}

template <class T>
template <class DB, class Y>
inline QLazySharedPointer<T>::
QLazySharedPointer (DB& db, const QSharedPointer<Y>& r)
    : p_ (r)
{
  if (p_)
    i_.reset_db (db);
}

template <class T>
template <class DB, class Y>
inline QLazySharedPointer<T>::
QLazySharedPointer (DB& db, const QWeakPointer<Y>& r)
    : p_ (r)
{
  if (p_)
    i_.reset_db (db);
}

template <class T>
inline typename QLazySharedPointer<T>::database_type& QLazySharedPointer<T>::
database () const
{
  return *i_.database ();
}

template <class T>
template <class O>
inline typename odb::object_traits<O>::id_type QLazySharedPointer<T>::
objectId () const
{
  typedef typename odb::object_traits<T>::object_type object_type;

  return p_
    ? odb::object_traits<object_type>::id (*p_)
    : i_.template object_id<O> ();
}

//
// QLazySharedPointer related non-member function definitions.
//

template <class T, class X>
inline QLazySharedPointer<X>
qSharedPointerCast (const QLazySharedPointer<T>& r)
{
  return r.template staticCast<X> ();
}

template <class T, class X>
inline QLazySharedPointer<X>
qSharedPointerConstCast (const QLazySharedPointer<T>& r)
{
  return r.template constCast<X> ();
}

template <class T, class X>
inline QLazySharedPointer<X>
qSharedPointerDynamicCast (const QLazySharedPointer<T>& r)
{
  return r.template dynamicCast<X> ();
}

template <class T, class X>
inline bool
operator== (const QLazySharedPointer<T>& a, const QLazySharedPointer<X>& b)
{
  return a.equal (b);
}

template <class T, class X>
inline bool
operator!= (const QLazySharedPointer<T>& a, const QLazySharedPointer<X>& b)
{
  return !a.equal (b);
}

//
// QLazyWeakPointer definition.
//

template <class T>
inline QLazyWeakPointer<T>::
QLazyWeakPointer () {}


template <class T>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (const QLazyWeakPointer& r):  p_ (r.p_), i_ (r.i_) {}

template <class T>
template <class X>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (const QLazyWeakPointer<X>& r): p_ (r.p_), i_ (r.i_) {}

template <class T>
template <class X>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (const QLazySharedPointer<X>& r): p_ (r.p_), i_ (r.i_) {}

template <class T>
inline QLazyWeakPointer<T>::
~QLazyWeakPointer () {}

template <class T>
inline QLazyWeakPointer<T>& QLazyWeakPointer<T>::
operator= (const QLazyWeakPointer& r)
{
  p_ = r.p_;
  i_ = r.i_;
  return *this;
}

template <class T>
template <class X>
inline QLazyWeakPointer<T>& QLazyWeakPointer<T>::
operator= (const QLazyWeakPointer<X>& r)
{
  p_ = r.p_;
  i_ = r.i_;
  return *this;
}

template <class T>
template <class X>
inline QLazyWeakPointer<T>& QLazyWeakPointer<T>::
operator= (const QLazySharedPointer<X>& r)
{
  p_ = r.p_;
  i_ = r.i_;
  return *this;
}

template <class T>
inline bool QLazyWeakPointer<T>::
operator! () const
{
  return isNull ();
}

#ifdef QWEAKPOINTER_ENABLE_ARROW
template <class T>
inline T* QLazyWeakPointer<T>::
operator-> () const
{
  return p_.operator-> ();
}
#endif

template <class T>
inline void QLazyWeakPointer<T>::
clear ()
{
  p_.clear ();
  i_.reset ();
}

template <class T>
inline T* QLazyWeakPointer<T>::
data () const
{
  return p_.data ();
}

template <class T>
inline bool QLazyWeakPointer<T>::
isNull () const
{
  return !(p_ || i_);
}

template <class T>
template <class X>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (const QWeakPointer<X>& r): p_ (r) {}

template <class T>
template <class X>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (const QSharedPointer<X>& r): p_ (r) {}

template <class T>
template <class X>
inline QLazyWeakPointer<T>& QLazyWeakPointer<T>::
operator= (const QWeakPointer<X>& r)
{
  p_ = r;
  i_.reset ();
  return *this;
}

template <class T>
template <class X>
inline QLazyWeakPointer<T>& QLazyWeakPointer<T>::
operator= (const QSharedPointer<X>& r)
{
  p_ = r;
  i_.reset ();
  return *this;
}

template <class T>
inline bool QLazyWeakPointer<T>::
loaded () const
{
  bool i (i_);
  return p_.toStrongRef ().isNull () != i; // expired () XOR i_
}

template <class T>
inline QSharedPointer<T> QLazyWeakPointer<T>::
load () const
{
  QSharedPointer<T> r (p_.toStrongRef ());

  if (!r && i_)
  {
    r = i_.template load<T> (false); // Keep id.
    p_ = r;
  }

  return r;
}

template <class T>
inline void QLazyWeakPointer<T>::
unload () const
{
  // With weak pointer we always keep i_ up to date.
  //
  p_.clear ();
}

template <class T>
inline QWeakPointer<T> QLazyWeakPointer<T>::
getEager () const
{
  return p_;
}

template <class T>
template <class DB, class ID>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (DB& db, const ID& id): i_ (db, id) {}

template <class T>
template <class DB, class X>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (DB& db, const QSharedPointer<X>& r)
  : p_ (r)
{
  typedef typename odb::object_traits<T>::object_type object_type;

  if (r)
    i_.reset (db, odb::object_traits<object_type>::id (*r));
}

template <class T>
template <class DB, class X>
inline QLazyWeakPointer<T>::
QLazyWeakPointer (DB& db, const QWeakPointer<X>& r)
    : p_ (r)
{
  typedef typename odb::object_traits<T>::object_type object_type;

  QSharedPointer<T> sp (p_.toStrongRef ());

  if (sp)
    i_.reset (db, odb::object_traits<object_type>::id (*sp));
}

template <class T>
template <class O /* = T */>
inline typename odb::object_traits<O>::id_type QLazyWeakPointer<T>::
objectId () const
{
  typedef typename odb::object_traits<T>::object_type object_type;

  QSharedPointer<T> sp (p_.toStrongRef ());

  return sp
    ? odb::object_traits<object_type>::id (*sp)
    : i_.template object_id<O> ();
}

template <class T>
inline typename QLazyWeakPointer<T>::database_type& QLazyWeakPointer<T>::
database () const
{
  return *i_.database ();
}

//
// QLazyWeakPointer related non-member functions.
//

template <class T, class X>
inline QLazySharedPointer<X>
qSharedPointerCast (const QLazyWeakPointer<T>& r)
{
  return QLazySharedPointer<T> (r).template staticCast<X> ();
}

template <class T, class X>
inline QLazySharedPointer<X>
qSharedPointerConstCast (const QLazyWeakPointer<T>& r)
{
  return QLazySharedPointer<T> (r).template constCast<X> ();
}

template <class T, class X>
inline QLazySharedPointer<X>
qSharedPointerDynamicCast (const QLazyWeakPointer<T>& r)
{
  return QLazySharedPointer<T> (r).template dynamicCast<X> ();
}

template <class T, class X>
inline QLazyWeakPointer<X>
qWeakPointerCast (const QLazyWeakPointer<T>& r)
{
  return QLazySharedPointer<T> (r).template staticCast<X> ().toWeakRef ();
}

template <class T, class X>
inline bool
operator== (const QLazyWeakPointer<T>& t, const QLazyWeakPointer<X>& x)
{
  return t.equal (x);
}

template <class T, class X>
inline bool
operator== (const QLazyWeakPointer<T>& t, const QLazySharedPointer<X>& x)
{
  return t.equal (x);
}

template <class T, class X>
inline bool
operator== (const QLazySharedPointer<T>& t, const QLazyWeakPointer<X>& x)
{
  return x.equal (t);
}

template <class T, class X>
inline bool
operator!= (const QLazyWeakPointer<T>& t, const QLazyWeakPointer<X>& x)
{
  return !t.equal (x);
}

template <class T, class X>
inline bool
operator!= (const QLazyWeakPointer<T>& t, const QLazySharedPointer<X>& x)
{
  return !t.equal (x);
}

template <class T, class X>
inline bool
operator!= (const QLazySharedPointer<T>& t, const QLazyWeakPointer<X>& x)
{
  return !x.equal (t);
}
