// file      : odb/qt/smart-ptr/lazy-ptr.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_QT_SMART_PTR_LAZY_PTR_HXX
#define ODB_QT_SMART_PTR_LAZY_PTR_HXX

#include <odb/pre.hxx>

#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>
#include <QtCore/QGlobalStatic>

#include <odb/forward.hxx> // odb::database
#include <odb/traits.hxx>
#include <odb/lazy-ptr-impl.hxx>
#include <odb/details/config.hxx> // ODB_CXX11

template <class T>
class QLazyWeakPointer;

//
// QLazySharedPointer declaration.
//

template <class T>
class QLazySharedPointer
{
  // The standard QSharedPointer interface.
  //
public:
  // These typedefs are inherited by QSharedPointer indirectly from
  // QtSharedPointer::Basic<T>
  //
  typedef T Type;
  typedef T element_type;
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef qptrdiff difference_type;

  QLazySharedPointer ();

  explicit
  QLazySharedPointer (T*);

  template <class Deleter>
  QLazySharedPointer (T*, Deleter);

  QLazySharedPointer (const QLazySharedPointer&);

  template <class X>
  QLazySharedPointer (const QLazySharedPointer<X>&);

  template <class X>
  QLazySharedPointer (const QLazyWeakPointer<X>&);

  ~QLazySharedPointer ();

  QLazySharedPointer&
  operator= (const QLazySharedPointer&);

  template <class X>
  QLazySharedPointer&
  operator= (const QLazySharedPointer<X>&);

  template <class X>
  QLazySharedPointer&
  operator= (const QLazyWeakPointer<X>&);

  typedef QSharedPointer<T> QLazySharedPointer<T>::*unspecified_bool_type;
  operator unspecified_bool_type () const
  {
    return isNull () ? 0 : &QLazySharedPointer::p_;
  }

  bool
  operator! () const;

  T&
  operator* () const;

  T*
  operator-> () const;

  void
  swap (QLazySharedPointer&);

  bool
  isNull () const;

  T*
  data () const;

  QLazyWeakPointer<T>
  toWeakRef () const;

  void
  clear ();

  template <class X>
  QLazySharedPointer<X>
  staticCast () const;

  template <class X>
  QLazySharedPointer<X>
  dynamicCast () const;

  template <class X>
  QLazySharedPointer<X>
  constCast () const;

  // Initialization and assignment from QSharedPointer and QWeakPointer.
  //
public:
  template <class X>
  QLazySharedPointer (const QSharedPointer<X>&);

  template <class X>
  explicit
  QLazySharedPointer (const QWeakPointer<X>&);

  template <class X>
  QLazySharedPointer&
  operator= (const QSharedPointer<X>&);

  template <class X>
  QLazySharedPointer&
  operator= (const QWeakPointer<X>&);

  // Lazy loading interface.
  //
public:
  typedef odb::database database_type;

  // isNull()   loaded()
  //
  //  true       true      NULL pointer to transient object
  //  false      true      valid pointer to persistent object
  //  true       false     unloaded pointer to persistent object
  //  false      false     valid pointer to transient object
  //
  bool
  loaded () const;

  QSharedPointer<T>
  load () const;

  // Unload the pointer. For transient objects this function is
  // equivalent to clear().
  //
  void unload () const;

  // Get the underlying eager pointer. If this is an unloaded pointer
  // to a persistent object, then the returned pointer will be NULL.
  //
  QSharedPointer<T>
  getEager () const;

  template <class DB, class ID>
  QLazySharedPointer (DB&, const ID&);

  template <class DB>
  QLazySharedPointer (DB&, T*);

  template <class DB, class Deleter>
  QLazySharedPointer (DB&, T*, Deleter);

  template <class DB, class X>
  QLazySharedPointer (DB&, const QSharedPointer<X>&);

  template <class DB, class X>
  QLazySharedPointer (DB&, const QWeakPointer<X>&);

#ifdef ODB_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGUMENT
  template <class O = T>
#else
  template <class O /* = T */>
#endif
  typename odb::object_traits<O>::id_type
  objectId () const;

  database_type&
  database () const;

  // Helpers.
  //
public:
  template <class X>
  bool
  equal (const QLazySharedPointer<X>&) const;

private:
  template <class X> friend class QLazySharedPointer;
  template <class X> friend class QLazyWeakPointer;

  mutable QSharedPointer<T> p_;
  mutable odb::lazy_ptr_impl<T> i_;
};

//
// QLazySharedPointer related non-members.
//

template <class T, class X>
QLazySharedPointer<X>
qSharedPointerCast (const QLazySharedPointer<T>&);

template <class T, class X>
QLazySharedPointer<X>
qSharedPointerConstCast (const QLazySharedPointer<T>&);

template <class T, class X>
QLazySharedPointer<X>
qSharedPointerDynamicCast (const QLazySharedPointer<T>&);

template <class T, class X>
bool
operator== (const QLazySharedPointer<T>&, const QLazySharedPointer<X>&);

template <class T, class X>
bool
operator!= (const QLazySharedPointer<T>&, const QLazySharedPointer<T>&);

//
// QLazyWeakPointer declaration
//

template <class T>
class QLazyWeakPointer
{
  // The standard QWeakPointer interface
  //
public:
  typedef T element_type;
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef qptrdiff difference_type;

  QLazyWeakPointer ();

  QLazyWeakPointer (const QLazyWeakPointer&);

  template <class X>
  QLazyWeakPointer (const QLazyWeakPointer<X>&);

  template <class X>
  QLazyWeakPointer (const QLazySharedPointer<X>&);

  ~QLazyWeakPointer ();

  QLazyWeakPointer&
  operator= (const QLazyWeakPointer&);

  template <class X>
  QLazyWeakPointer&
  operator= (const QLazyWeakPointer<X>&);

  template <class X>
  QLazyWeakPointer&
  operator= (const QLazySharedPointer<X>&);

  typedef QWeakPointer<T> QLazyWeakPointer<T>::*unspecified_bool_type;
  operator unspecified_bool_type () const
  {
    return isNull () ? 0 : &QLazyWeakPointer<T>::p_;
  }

  bool
  operator! () const;

#ifdef QWEAKPOINTER_ENABLE_ARROW
  T*
  operator-> () const;
#endif

  void
  clear ();

  T*
  data () const;

  bool
  isNull () const;

  QLazySharedPointer<T>
  toStrongRef () const;

  // Initialization/assignment from QSharedPointer and QWeakPointer.
  //
public:
  template <class X>
  QLazyWeakPointer (const QWeakPointer<X>&);

  template <class X>
  QLazyWeakPointer (const QSharedPointer<X>&);

  template <class X>
  QLazyWeakPointer&
  operator= (const QWeakPointer<X>&);

  template <class X>
  QLazyWeakPointer&
  operator= (const QSharedPointer<X>&);

  // Lazy loading interface.
  //
public:
  typedef odb::database database_type;

  // toStrongRef().isNull()  loaded()
  //
  //          true             true      expired pointer to transient object
  //          false            true      valid pointer to persistent object
  //          true             false     expired pointer to persistent object
  //          false            false     valid pointer to transient object
  //
  bool
  loaded () const;

  // Create a strong reference using toStrongRef() and load.
  //
  QSharedPointer<T>
  load () const;

  // Unload the pointer. For transient objects this function is equivalent
  // to clear().
  //
  void
  unload () const;

  // Get the underlying eager pointer. If this is an unloaded pointer
  // to a persistent object, then the returned pointer will be NULL.
  //
  QWeakPointer<T>
  getEager () const;

  template <class DB, class ID>
  QLazyWeakPointer (DB&, const ID&);

  template <class DB, class X>
  QLazyWeakPointer (DB&, const QSharedPointer<X>&);

  template <class DB, class X>
  QLazyWeakPointer (DB&, const QWeakPointer<X>&);

  // The objectId() function can only be called when the object is persistent,
  // or: toStrongRef().isNull() XOR loaded() (can use != for XOR).
  //
#ifdef ODB_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGUMENT
  template <class O = T>
#else
  template <class O /* = T */>
#endif
  typename odb::object_traits<O>::id_type
  objectId () const;

  database_type&
  database () const;

  // Helpers.
  //
public:
  template <class X>
  bool
  equal (const QLazyWeakPointer<X>&) const;

  template <class X>
  bool
  equal (const QLazySharedPointer<X>&) const;

private:
  template <class X> friend class QLazySharedPointer;
  template <class X> friend class QLazyWeakPointer;

  mutable QWeakPointer<T> p_;
  mutable odb::lazy_ptr_impl<T> i_;
};

//
// QLazyWeakPointer related non-members.
//

template <class T, class X>
QLazySharedPointer<X>
qSharedPointerCast (const QLazyWeakPointer<T>&);

template <class T, class X>
QLazySharedPointer<X>
qSharedPointerConstCast (const QLazyWeakPointer<T>&);

template <class T, class X>
QLazySharedPointer<X>
qSharedPointerDynamicCast (const QLazyWeakPointer<T>&);

template <class T, class X>
QLazyWeakPointer<X>
qWeakPointerCast (const QLazyWeakPointer<T>&);

template <class T, class X>
bool
operator== (const QLazyWeakPointer<T>&, const QLazyWeakPointer<X>&);

template <class T, class X>
bool
operator== (const QLazyWeakPointer<T>&, const QLazySharedPointer<X>&);

template <class T, class X>
bool
operator== (const QLazySharedPointer<T>&, const QLazyWeakPointer<X>&);

template <class T, class X>
bool
operator!= (const QLazyWeakPointer<T>&, const QLazyWeakPointer<X>&);

template <class T, class X>
bool
operator!= (const QLazyWeakPointer<T>&, const QLazySharedPointer<X>&);

template <class T, class X>
bool
operator!= (const QLazySharedPointer<T>&, const QLazyWeakPointer<X>&);

#include <odb/qt/smart-ptr/lazy-ptr.ixx>
#include <odb/qt/smart-ptr/lazy-ptr.txx>

#include <odb/qt/smart-ptr/lazy-pointer-traits.hxx>

#include <odb/post.hxx>

#endif // ODB_QT_SMART_PTR_LAZY_PTR_HXX
