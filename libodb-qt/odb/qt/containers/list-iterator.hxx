// file      : odb/qt/containers/list-iterator.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINERS_LIST_ITERATOR_HXX
#define ODB_QT_CONTAINERS_LIST_ITERATOR_HXX

#include <odb/pre.hxx>

#include <QtCore/QListIterator>

#include <odb/qt/list.hxx>

// Java-style QListIterator-like iterator. You can also use the
// QListIterator directly (but not QMutableListIterator).
//
template <typename T>
class QOdbListIterator: public QListIterator<T>
{
public:
  QOdbListIterator (const QOdbList<T>& c): QListIterator<T> (c) {}
  QOdbListIterator& operator=(const QOdbList<T>& c)
  {
    static_cast<QListIterator<T>&> (*this) = c;
    return *this;
  }
};

#include <odb/post.hxx>

#endif // ODB_QT_CONTAINERS_LIST_ITERATOR_HXX
