// file      : odb/qt/containers/list.ixx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

//
// QOdbList
//

template <typename T>
inline QOdbList<T>& QOdbList<T>::
operator= (const QOdbList<T>& x)
{
  l_ = x.l_;
  if (_tracking ())
    impl_.assign (static_cast<std::size_t> (l_.size ()));
  return *this;
}

template <typename T>
inline QOdbList<T>& QOdbList<T>::
operator= (const base_list_type& x)
{
  l_ = x;
  if (_tracking ())
    impl_.assign (static_cast<std::size_t> (l_.size ()));
  return *this;
}

#ifdef ODB_CXX11
template <typename T>
inline QOdbList<T>& QOdbList<T>::
operator= (QOdbList&& x)
{
  l_ = std::move (x.l_);
  if (_tracking ())
    impl_.assign (static_cast<std::size_t> (l_.size ()));
  return *this;
}

template <typename T>
inline QOdbList<T>& QOdbList<T>::
operator= (base_list_type&& x)
{
  l_ = std::move (x);
  if (_tracking ())
    impl_.assign (static_cast<std::size_t> (l_.size ()));
  return *this;
}
#endif

#if QT_VERSION >= 0x040800
template <typename T>
inline void QOdbList<T>::
swap (QOdbList<T>& x)
{
  l_.swap (x.l_);
  vector_base::swap (x);
}
#endif

template <typename T>
inline void QOdbList<T>::
clear()
{
  l_.clear ();
  if (_tracking ())
    impl_.clear ();
}

template <typename T>
inline T& QOdbList<T>::
modify (int i)
{
  T& r (l_[i]);
  if (_tracking ())
    impl_.modify (static_cast<std::size_t> (i));
  return r;
}

template <typename T>
inline void QOdbList<T>::
reserve (int n)
{
  l_.reserve (n);
  if (_tracking ())
    impl_.reserve (static_cast<std::size_t> (n));
}

template <typename T>
inline void QOdbList<T>::
append (const T& x)
{
  l_.append (x);
  if (_tracking ())
    impl_.push_back ();
}

template <typename T>
inline void QOdbList<T>::
append (const QList<T>& x)
{
  l_.append (x);
  if (_tracking ())
    impl_.push_back (static_cast<std::size_t> (x.size ()));
}

template <typename T>
inline void QOdbList<T>::
prepend (const T& x)
{
  l_.prepend (x);
  if (_tracking ())
    impl_.insert (0);
}

template <typename T>
inline void QOdbList<T>::
insert (int i, const T& x)
{
  l_.insert (i, x);
  if (_tracking ())
    impl_.insert (static_cast<std::size_t> (i));
}

template <typename T>
inline void QOdbList<T>::
replace (int i, const T& x)
{
  l_.insert (i, x);
  if (_tracking ())
    impl_.modify (static_cast<std::size_t> (i));
}

template <typename T>
inline void QOdbList<T>::
removeAt (int i)
{
  l_.removeAt (i);
  if (_tracking ())
    impl_.erase (static_cast<std::size_t> (i));
}

template <typename T>
inline int QOdbList<T>::
removeAll (const T& x)
{
  // We have to re-implement this one ourselves since we need to
  // know the indexes of the removed elements.
  //
  int r (0);
  for (int i (l_.indexOf (x)); i != -1; i = l_.indexOf (x, i))
  {
    removeAt (i);
    r++;
  }
  return r;
}

template <typename T>
inline bool QOdbList<T>::
removeOne (const T& x)
{
  // We have to re-implement this one ourselves since we need to
  // know the index of the removed element.
  //
  int i (l_.indexOf (x));
  if (i != -1)
    removeAt (i);
  return i != -1;
}

template <typename T>
inline T QOdbList<T>::
takeAt (int i)
{
  if (_tracking ())
    impl_.erase (static_cast<std::size_t> (i));
  return l_.takeAt (i);
}

template <typename T>
inline T QOdbList<T>::
takeFirst ()
{
  if (_tracking ())
    impl_.erase (0);
  return l_.takeFirst ();
}

template <typename T>
inline T QOdbList<T>::
takeLast ()
{
  if (_tracking ())
    impl_.pop_back ();
  return l_.takeLast ();
}

template <typename T>
inline void QOdbList<T>::
move (int from, int to)
{
  l_.move (from, to);
  if (_tracking ())
  {
    impl_.erase (static_cast<std::size_t> (from));
    impl_.insert (static_cast<std::size_t> (to));
  }
}

template <typename T>
inline void QOdbList<T>::
swap (int i, int j)
{
  l_.swap (i, j);
  if (_tracking ())
  {
    impl_.modify (static_cast<std::size_t> (i));
    impl_.modify (static_cast<std::size_t> (j));
  }
}

template <typename T>
inline typename QOdbList<T>::base_iterator_type QOdbList<T>::
mbegin ()
{
  if (_tracking ())
    impl_.modify (0, static_cast<std::size_t> (l_.size ()));
  return l_.begin ();
}

template <typename T>
inline typename QOdbList<T>::iterator QOdbList<T>::
insert (iterator p, const T& x)
{
  if (_tracking ())
    impl_.insert (static_cast<std::size_t> (p.base () - l_.begin ()));
  return iterator (this, l_.insert (p.base (), x));
}

template <typename T>
inline typename QOdbList<T>::iterator QOdbList<T>::
erase (iterator p)
{
  if (_tracking ())
    impl_.erase (static_cast<std::size_t> (p.base () - l_.begin ()));
  return iterator (this, l_.erase (p.base ()));
}

template <typename T>
inline typename QOdbList<T>::iterator QOdbList<T>::
erase (iterator f, iterator l)
{
  if (_tracking ())
    impl_.erase (static_cast<std::size_t> (f.base () - l_.begin ()),
                 static_cast<std::size_t> (l - f));
  return iterator (this, l_.erase (f.base (), l.base ()));
}

template <typename T>
inline T& QOdbList<T>::
modifyFirst ()
{
  T& r (l_.first ());
  if (_tracking ())
    impl_.modify (0);
  return r;
}

template <typename T>
inline T& QOdbList<T>::
modifyLast ()
{
  T& r (l_.last ());
  if (_tracking ())
    impl_.modify (static_cast<std::size_t> (l_.size () - 1));
  return r;
}

template <typename T>
inline void QOdbList<T>::
removeFirst ()
{
  l_.removeFirst ();
  if (_tracking ())
    impl_.erase (0);
}

template <typename T>
inline void QOdbList<T>::
removeLast ()
{
  l_.removeLast ();
  if (_tracking ())
    impl_.pop_back ();
}

//
// QOdbListIteratorImpl
//

template <typename L>
inline typename QOdbListIteratorImpl<L>::reference QOdbListIteratorImpl<L>::
modify () const
{
  if (l_->_tracking ())
    l_->_impl ().modify (static_cast<std::size_t> (i_ - l_->base ().begin ()));
  return *i_;
}

template <typename L>
inline typename QOdbListIteratorImpl<L>::reference QOdbListIteratorImpl<L>::
modify (difference_type n) const
{
  if (l_->_tracking ())
    l_->_impl ().modify (
      static_cast<std::size_t> (i_ - l_->base ().begin () + n));
  return i_[n];
}
