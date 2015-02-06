// file      : common/session/custom/session.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef SESSION_HXX
#define SESSION_HXX

#include <map>
#include <memory>
#include <typeinfo>

#include <odb/database.hxx>
#include <odb/transaction.hxx>

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

  // Session for the current thread. This can be implemented in pretty
  // much any way that makes sense to the application. It can be a global
  // session as we have here. In multi-threaded applications we could use
  // TLS instead.
  //
public:
  static session* current;

  // Change tracking interface.
  //
public:
  // Call flush() within a transaction to apply the changes to the
  // database.
  //
  void
  flush (odb::database&);

private:
  struct object_map_base
  {
    virtual
    ~object_map_base () {}

    // Return true if we flushed anything.
    //
    virtual bool
    flush (odb::database&) = 0;

    virtual void
    mark (unsigned short event) = 0;
  };

  enum object_state
  {
    tracking, // Tracking any modifications by storing the original copy.
    changed,  // Known to be changed.
    flushed   // Flushed but not yet committed/rolled back.
  };

  template <typename T>
  struct object_data
  {
    typedef typename odb::object_traits<T>::pointer_type pointer_type;

    explicit
    object_data (pointer_type o): obj (o), state (tracking) {}

    pointer_type obj;
    pointer_type orig;
    object_state state;
  };

  template <typename T>
  struct object_map: object_map_base,
                     std::map<typename odb::object_traits<T>::id_type,
                              object_data<T> >
  {
    virtual bool
    flush (odb::database&);

    virtual void
    mark (unsigned short event);
  };

  // Object cache interface.
  //
public:
  template <typename T>
  struct cache_position
  {
    typedef object_map<T> map;
    typedef typename map::iterator iterator;

    cache_position (): map_ (0) {}
    cache_position (map& m, const iterator& p): map_ (&m), pos_ (p) {}

    cache_position&
    operator= (const cache_position& p)
    {
      // It might not be ok to use an uninitialized iterator on the rhs.
      //
      if (p.map_ != 0)
        pos_ = p.pos_;
      map_ = p.map_;
      return *this;
    }

    map* map_;
    iterator pos_;
  };

  // Cache management.
  //
  template <typename T>
  static cache_position<T>
  _cache_insert (odb::database&,
                 const typename odb::object_traits<T>::id_type&,
                 const typename odb::object_traits<T>::pointer_type&);

  template <typename T>
  static typename odb::object_traits<T>::pointer_type
  _cache_find (odb::database&, const typename odb::object_traits<T>::id_type&);

  template <typename T>
  static void
  _cache_erase (const cache_position<T>& p)
  {
    if (p.map_ != 0)
      p.map_->erase (p.pos_);
  }

  // Notifications.
  //
  template <typename T>
  static void
  _cache_persist (const cache_position<T>& p)
  {
    _cache_load (p);
  }

  template <typename T>
  static void
  _cache_load (const cache_position<T>&);

  template <typename T>
  static void
  _cache_update (odb::database&, const T&);

  template <typename T>
  static void
  _cache_erase (odb::database&,
                const typename odb::object_traits<T>::id_type&);

private:
  // Post-commit/rollback callback.
  //
  static void
  mark (unsigned short event, void* key, unsigned long long);

private:
  typedef std::map<const std::type_info*,
                   std::shared_ptr<object_map_base>,
                   odb::details::type_info_comparator> type_map;
  type_map map_;
  odb::transaction* tran_;
};

#include "session.txx"

#endif // SESSION_HXX
