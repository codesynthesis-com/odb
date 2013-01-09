// file      : common/session/custom/session.hxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef SESSION_HXX
#define SESSION_HXX

#include <map>
#include <memory>
#include <typeinfo>

#include <odb/database.hxx>
#include <odb/traits.hxx>            // odb::object_traits
#include <odb/details/type-info.hxx> // odb::details::type_info_comparator

// This custom session implementation assumes we are working with
// one database at a time.
//
class session
{
public:
  session ();
  ~session ();

private:
  session (const session&);
  session& operator= (const session&);

  // Current session interface.
  //
public:
  static bool
  has_current ();

  static session&
  current ();

  // Change tracking interface.
  //
  // Call flush() within a transaction to apply the changes to the
  // database. Then after successfully committing the transaction,
  // call mark() to mark all the changed objects as again unchanged.
  //
public:
  void
  flush (odb::database&);

  void
  mark ();

private:
  struct object_map_base
  {
    virtual
    ~object_map_base () {}

    virtual void
    flush (odb::database&) = 0;

    virtual void
    mark () = 0;
  };

  template <typename T>
  struct object_state
  {
    typedef typename odb::object_traits<T>::pointer_type pointer_type;

    explicit
    object_state (pointer_type o): obj (o), flushed_ (false) {}

    pointer_type obj;
    pointer_type orig;
    bool flushed_;
  };

  template <typename T>
  struct object_map: object_map_base,
                     std::map<typename odb::object_traits<T>::id_type,
                              object_state<T> >
  {
    virtual void
    flush (odb::database&);

    virtual void
    mark ();
  };

  // Object cache interface.
  //
public:
  template <typename T>
  struct position
  {
    typedef object_map<T> map;
    typedef typename map::iterator iterator;

    position () {}
    position (map& m, const iterator& p): map_ (&m), pos_ (p) {}

    map* map_;
    iterator pos_;
  };

  template <typename T>
  position<T>
  insert (odb::database&,
          const typename odb::object_traits<T>::id_type&,
          const typename odb::object_traits<T>::pointer_type&);

  template <typename T>
  static void
  initialize (const position<T>&);

  template <typename T>
  typename odb::object_traits<T>::pointer_type
  find (odb::database&, const typename odb::object_traits<T>::id_type&) const;

  template <typename T>
  void
  erase (odb::database&, const typename odb::object_traits<T>::id_type&);

  template <typename T>
  static void
  erase (const position<T>& p)
  {
    p.map_->erase (p.pos_);
  }

private:
  typedef std::map<const std::type_info*,
                   std::shared_ptr<object_map_base>,
                   odb::details::type_info_comparator> type_map;
  type_map map_;
};

#include "session.txx"

#endif // SESSION_HXX
