// file      : odb/qt/containers/mutable-list-iterator.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINERS_MUTABLE_LIST_ITERATOR_HXX
#define ODB_QT_CONTAINERS_MUTABLE_LIST_ITERATOR_HXX

#include <odb/pre.hxx>

#include <QtCore/QMutableListIterator>

#include <odb/qt/list.hxx>

// Java-style QMutableListIterator-like iterator. The implementation
// is based on what's found in qiterator.h.
//
template <typename T>
class QMutableOdbListIterator
{
public:
  QMutableOdbListIterator(QOdbList<T> &x)
      : c (&x)
  {
    c->setSharable(false);
    i = c->begin();
    n = c->end();
  }

  ~QMutableOdbListIterator() {c->setSharable(true);}

  QMutableOdbListIterator &operator=(QOdbList<T> &x)
  {
    c->setSharable(true);
    c = &x;
    c->setSharable(false);
    i = c->begin();
    n = c->end();
    return *this;
  }

  void toFront() {i = c->begin(); n = c->end();}
  void toBack() {i = c->end(); n = i;}
  bool hasNext() const {return c->constEnd() != const_iterator(i.base ());}
  bool hasPrevious() const
  {
    return c->constBegin() != const_iterator(i.base ());
  }

  bool findNext(const T &t)
  {
    while (c->constEnd() != const_iterator((n = i).base ()))
      if (*i++ == t)
        return true;
    return false;
  }

  bool findPrevious(const T &t)
  {
    while (c->constBegin() != const_iterator(i.base ()))
      if (*(n = --i) == t)
        return true;

    n = c->end();
    return false;
  }

  T &next() {n = i++; return n.modify ();}
  T &peekNext() const {return i.modify ();}
  T &previous() {n = --i; return n.modify ();}
  T &peekPrevious() const {iterator p (i); return (--p).modify ();}

  void remove()
  {
    if (c->constEnd() != const_iterator(n.base ()))
    {
      i = c->erase (n);
      n = c->end();
    }
  }

  void setValue(const T &t) const
  {
    if (c->constEnd() != const_iterator(n.base ()))
      n.modify () = t;
  }

  T &value()
  {
    Q_ASSERT(c->constEnd() != const_iterator(n.base ()));
    return n.modify ();
  }

  const T &value() const
  {
    Q_ASSERT(c->constEnd() != const_iterator(n.base ()));
    return *n;
  }

  void insert(const T &t) {n = i = c->insert(i, t); ++i;}

private:
  typedef typename QOdbList<T>::iterator iterator;
  typedef typename QOdbList<T>::const_iterator const_iterator;

  QOdbList<T>* c;
  iterator i, n;
};

#include <odb/post.hxx>

#endif // ODB_QT_CONTAINERS_MUTABLE_LIST_ITERATOR_HXX
