// file      : odb/qt/containers/list.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_CONTAINERS_LIST_HXX
#define ODB_QT_CONTAINERS_LIST_HXX

#include <odb/pre.hxx>
#include <odb/details/config.hxx> // ODB_CXX11

#include <QtCore/QtGlobal> // QT_VERSION
#include <QtCore/QList>

#ifdef ODB_CXX11
#  include <utility>          // std::move
#  if defined(ODB_CXX11_INITIALIZER_LIST) && \
  defined(Q_COMPILER_INITIALIZER_LISTS)
#    include <initializer_list>
#  endif
#endif

#include <odb/vector-impl.hxx>

// A QList-like container that keeps track of changes.
//
// Note that the style and order of definitions is (mostly) as
// appears in the qlist.h Qt header (except for some cleanups,
// such as superfluous inline use).
//
template <typename L>
class QOdbListIteratorImpl;

template <typename T>
class QOdbList: public odb::vector_base
{
public:
  typedef QList<T> base_list_type;
  typedef typename base_list_type::iterator base_iterator_type;

  QOdbList() {}
  QOdbList(const QOdbList &x): vector_base (x), l_ (x.l_) {}
  // ~QOdbList();
  QOdbList &operator=(const QOdbList &l);

#if QT_VERSION >= 0x040800
  void swap(QOdbList &other);
#endif

#ifdef ODB_CXX11
  QOdbList(QOdbList &&x): vector_base (std::move (x)), l_ (std::move (x.l_)) {}
  QOdbList &operator=(QOdbList &&other);
#if defined(ODB_CXX11_INITIALIZER_LIST) && \
  defined(Q_COMPILER_INITIALIZER_LISTS)
  QOdbList(std::initializer_list<T> il): l_ (il) {}
#endif
#endif

  // Implicit conversion.
  //
  bool operator==(const QList<T> &x) const {return l_ == x;}
  bool operator!=(const QList<T> &x) const {return l_ != x;}

  int size() const {return l_.size ();}
  void detach() {l_.detach ();}
  void detachShared() {l_.detachShared ();}
  bool isDetached() const {return l_.isDetached ();}
  void setSharable(bool sharable) {l_.setSharable (sharable);}
  // Implicit conversion.
  bool isSharedWith(const QList<T> &x) const {return l_.isSharedWith (x);}
  bool isEmpty() const {return l_.isEmpty ();}
  void clear();

  const T &at(int i) const {return l_.at (i);}
  const T &operator[](int i) const {return l_[i];}
  //T &operator[](int i);
  T &modify (int i);

  void reserve(int size);
  void append(const T &t);
  void append(const QList<T> &t); // Implicit conversion.
  void prepend(const T &t);
  void insert(int i, const T &t);
  void replace(int i, const T &t);
  void removeAt(int i);
  int removeAll(const T &t);
  bool removeOne(const T &t);
  T takeAt(int i);
  T takeFirst();
  T takeLast();
  void move(int from, int to);
  void swap(int i, int j);

  int indexOf(const T &t, int from = 0) const {return l_.indexOf (t, from);}
  int lastIndexOf(const T &t, int from = -1) const
  {return l_.lastIndexOf (t, from);}
  bool contains(const T &t) const {return l_.contains (t);}
  int count(const T &t) const {return l_.count (t);}

  typedef QOdbListIteratorImpl<QOdbList> iterator;
  typedef typename base_list_type::const_iterator const_iterator;

  // stl style
  iterator begin() {return iterator (this, l_.begin ());}
  const_iterator begin() const {return l_.begin ();}
  const_iterator cbegin() const {return l_.cbegin ();}
  const_iterator constBegin() const {return l_.constBegin ();}
  iterator end() {return iterator (this, l_.end ());}
  const_iterator end() const {return l_.end ();}
  const_iterator cend() const {return l_.cend ();}
  const_iterator constEnd() const {return l_.constEnd ();}

  // Return QList iterators. The begin() functions mark all
  // the elements as modified.
  //
  base_iterator_type mbegin ();
  base_iterator_type modifyBegin () {return mbegin ();}
  base_iterator_type mend () {return l_.end ();}
  base_iterator_type modifyEnd () {return mend ();}

  iterator insert(iterator before, const T &t);
  iterator erase(iterator pos);
  iterator erase(iterator first, iterator last);

  // more Qt
  typedef iterator Iterator;
  typedef const_iterator ConstIterator;

  int count() const {return l_.count ();}
  int length() const {return l_.length ();}
  //T& first();
  T& modifyFirst();
  const T& first() const {return l_.first ();}
  //T& last();
  T& modifyLast();
  const T& last() const {return l_.last ();}
  void removeFirst();
  void removeLast();
  bool startsWith(const T &t) const {return l_.startsWith (t);}
  bool endsWith(const T &t) const {return l_.endsWith (t);}
  QList<T> mid(int pos, int length = -1) const {return l_.mid (pos, length);}

  T value(int i) const {return l_.value (i);}
  T value(int i, const T &defValue) const {return l_.value (i, defValue);}

  // stl compatibility
  void push_back(const T &t) {append(t);}
  void push_front(const T &t) {prepend(t);}
  //T& front();
  T& modify_front() {return modifyFirst ();}
  const T& front() const {return l_.front ();}
  //T& back();
  T& modify_back() {return modifyLast ();}
  const T& back() const {return l_.back ();}
  void pop_front() {removeFirst();}
  void pop_back() {removeLast();}
  bool empty() const {return l_.empty ();}

  typedef int size_type;
  typedef T value_type;
  typedef value_type *pointer;
  typedef const value_type *const_pointer;
  typedef value_type &reference;
  typedef const value_type &const_reference;
  typedef typename base_list_type::difference_type difference_type;

  // comfort
  // Implicit conversion.
  QOdbList &operator+=(const QList<T> &l) {append (l); return *this;}
  QOdbList operator+(const QList<T> &l) const
  {QOdbList r (*this); r.append (l); return r;}
  QOdbList &operator+=(const T &t) {append (t); return *this;}
  QOdbList &operator<< (const T &t) {append (t); return *this;}
  QOdbList &operator<<(const QList<T> &l) {append (l); return *this;}

  QVector<T> toVector() const {return l_.toVector ();}
  QSet<T> toSet() const {return l_.toSet ();}

  static QOdbList fromVector(const QVector<T> &v)
  {return base_list_type::fromVector (v);}
  static QOdbList fromSet(const QSet<T> &s)
  {return base_list_type::fromSet (s);}

  static QOdbList fromStdList(const std::list<T> &l)
  {return base_list_type::fromStdList (l);}
  std::list<T> toStdList() const {return l_.toStdList ();}

  // Interfacing with the base list.
  //
  QOdbList (const base_list_type& x): l_ (x) {}
  QOdbList& operator= (const base_list_type&);
  operator const base_list_type& () const {return l_;}
  base_list_type& base () {return l_;}
  const base_list_type& base () const {return l_;}

#ifdef ODB_CXX11
  QOdbList (base_list_type&& x): l_ (std::move (x)) {}
  QOdbList& operator= (base_list_type&&);
#endif

  // Change tracking (the rest comes from vector_base).
  //
public:
  void _start () const {impl_.start (l_.size ());}

private:
  base_list_type l_;
};

template <typename L>
class QOdbListIteratorImpl
{
public:
  typedef L list_type;
  typedef typename list_type::base_iterator_type base_iterator_type;
  typedef typename list_type::const_iterator const_iterator_type;

  typedef typename base_iterator_type::iterator_category iterator_category;
  typedef typename base_iterator_type::difference_type difference_type;
  typedef typename base_iterator_type::value_type value_type;
  typedef typename base_iterator_type::pointer pointer;
  typedef typename base_iterator_type::reference reference;

  typedef typename list_type::size_type size_type;
  typedef typename list_type::const_reference const_reference;
  typedef typename list_type::const_pointer const_pointer;

  QOdbListIteratorImpl (): l_ (0), i_ () {}
  QOdbListIteratorImpl (list_type* l, const base_iterator_type& i)
      : l_ (l), i_ (i) {}

#ifndef QT_STRICT_ITERATORS
  operator const_iterator_type () const {return i_;}
#endif
  base_iterator_type base () const {return i_;}
  list_type* list () const {return l_;}

  // Note: const_{reference,pointer}.
  //
  const_reference operator* () const {return *i_;}
  const_pointer   operator-> () const {return i_.operator -> ();}
  const_reference operator[] (difference_type n) const {return i_[n];}

  // Modifiers.
  //
  reference modify () const;
  reference modify (difference_type n) const;

  QOdbListIteratorImpl& operator++ () {++i_; return *this;}
  QOdbListIteratorImpl  operator++ (int)
  {return QOdbListIteratorImpl (l_, i_++);}
  QOdbListIteratorImpl& operator-- () {--i_; return *this;}
  QOdbListIteratorImpl  operator-- (int)
  {return QOdbListIteratorImpl (l_, i_--);}

  QOdbListIteratorImpl operator+ (int n) const
  {return QOdbListIteratorImpl (l_, i_ + n);}
  QOdbListIteratorImpl& operator+= (int n) {i_ += n; return *this;}
  QOdbListIteratorImpl operator- (int n) const
  {return QOdbListIteratorImpl (l_, i_ - n);}
  QOdbListIteratorImpl& operator-= (int n) {i_ -= n; return *this;}

private:
  list_type* l_;
  base_iterator_type i_;
};

// operator==
//
template <typename L>
inline bool
operator== (const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () == y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline bool
operator== (const QOdbListIteratorImpl<L>& x,
            const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () == y;}

template <typename L>
inline bool
operator== (const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
            const QOdbListIteratorImpl<L>& y)
{return x == y.base ();}
#endif

// operator<
//
template <typename L>
inline bool
operator< (const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () < y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline bool
operator< (const QOdbListIteratorImpl<L>& x,
           const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () < y;}

template <typename L>
inline bool
operator< (const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
           const QOdbListIteratorImpl<L>& y)
{return x < y.base ();}
#endif

// operator!=
//
template <typename L>
inline bool
operator!= (const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () != y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline bool
operator!= (const QOdbListIteratorImpl<L>& x,
            const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () != y;}

template <typename L>
inline bool
operator!= (const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
            const QOdbListIteratorImpl<L>& y)
{return x != y.base ();}
#endif

// operator>
//
template <typename L>
inline bool
operator> (const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () > y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline bool
operator> (const QOdbListIteratorImpl<L>& x,
           const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () > y;}

template <typename L>
inline bool
operator> (const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
           const QOdbListIteratorImpl<L>& y)
{return x > y.base ();}
#endif

// operator>=
//
template <typename L>
inline bool
operator>= (const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () >= y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline bool
operator>= (const QOdbListIteratorImpl<L>& x,
            const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () >= y;}

template <typename L>
inline bool
operator>= (const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
            const QOdbListIteratorImpl<L>& y)
{return x >= y.base ();}
#endif

// operator<=
//
template <typename L>
inline bool
operator<= (const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () <= y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline bool
operator<= (const QOdbListIteratorImpl<L>& x,
            const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () <= y;}

template <typename L>
inline bool
operator<= (const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
            const QOdbListIteratorImpl<L>& y)
{return x <= y.base ();}
#endif

// operator-
//
template <typename L>
inline typename QOdbListIteratorImpl<L>::difference_type
operator-(const QOdbListIteratorImpl<L>& x, const QOdbListIteratorImpl<L>& y)
{return x.base () - y.base ();}

#ifndef QT_STRICT_ITERATORS
template <typename L>
inline typename QOdbListIteratorImpl<L>::difference_type
operator-(const QOdbListIteratorImpl<L>& x,
          const typename QOdbListIteratorImpl<L>::const_iterator_type& y)
{return x.base () - y;}

template <typename L>
inline typename QOdbListIteratorImpl<L>::difference_type
operator-(const typename QOdbListIteratorImpl<L>::const_iterator_type& x,
          const QOdbListIteratorImpl<L>& y)
{return x - y.base ();}
#endif

// operator+
//
template <typename L>
inline QOdbListIteratorImpl<L>
operator+(typename QOdbListIteratorImpl<L>::difference_type n,
          const QOdbListIteratorImpl<L>& x)
{return QOdbListIteratorImpl<L> (x.list (), n + x.base ());}

#include <odb/qt/containers/list.ixx>

#include <odb/qt/containers/list-traits.hxx>

#include <odb/post.hxx>

#endif // ODB_QT_CONTAINERS_LIST_HXX
