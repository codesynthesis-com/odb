// file      : odb/instance.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_INSTANCE_HXX
#define ODB_INSTANCE_HXX

#include <map>
#include <string>
#include <cstddef>  // std::size_t
#include <utility>  // std::forward()
#include <typeinfo>

#include <odb/option-types.hxx>
#include <odb/context.hxx>

#include <odb/traversal/elements.hxx>
#include <odb/traversal/relational/elements.hxx>

//
// Dynamic traversal instantiation support.
//

template <typename B>
struct factory
{
  static B*
  create (const B& prototype)
  {
    std::string kind, name;
    database db (context::current ().options.database ()[0]);

    switch (db)
    {
    case database::common:
      {
        name = "common";
        break;
      }
    case database::mssql:
    case database::mysql:
    case database::oracle:
    case database::pgsql:
    case database::sqlite:
      {
        kind = "relational";
        name = kind + "::" + db.string ();
        break;
      }
    }

    if (map_ != 0)
    {
      typename map::const_iterator i;

      if (!name.empty ())
        i = map_->find (name);

      if (i == map_->end ())
        i = map_->find (kind);

      if (i != map_->end ())
        return i->second (prototype);
    }

    return new B (prototype);
  }

private:
  template <typename>
  friend struct entry;

  static void
  init ()
  {
    if (factory<B>::count_++ == 0)
      factory<B>::map_ = new typename factory<B>::map;
  }

  static void
  term ()
  {
    if (--factory<B>::count_ == 0)
      delete factory<B>::map_;
  }

  typedef B* (*create_func) (const B&);
  typedef std::map<std::string, create_func> map;
  static map* map_;
  static std::size_t count_;
};

template <typename B>
typename factory<B>::map* factory<B>::map_;

template <typename B>
std::size_t factory<B>::count_;

struct entry_base
{
  static std::string
  name (std::type_info const&);
};

template <typename D>
struct entry: entry_base
{
  typedef typename D::base base;

  entry ()
  {
    factory<base>::init ();
    (*factory<base>::map_)[name (typeid (D))] = &create;
  }

  ~entry ()
  {
    factory<base>::term ();
  }

  static base*
  create (const base& prototype)
  {
    return new D (prototype);
  }
};

template <typename B>
struct instance
{
  typedef typename B::base base_type;
  typedef ::factory<base_type> factory_type;

  ~instance ()
  {
    delete x_;
  }

  template <typename... A>
  instance (A&&... a)
  {
    base_type prototype (std::forward<A> (a)...);
    x_ = factory_type::create (prototype);
  }

  instance (instance const& i)
  {
    // This is tricky: use the other instance as a prototype.
    //
    x_ = factory_type::create (*i.x_);
  }

  base_type*
  operator-> () const
  {
    return x_;
  }

  base_type&
  operator* () const
  {
    return *x_;
  }

  base_type*
  get () const
  {
    return x_;
  }

private:
  instance& operator= (instance const&);

private:
  base_type* x_;
};

template <typename T>
inline traversal::edge_base&
operator>> (instance<T>& n, traversal::edge_base& e)
{
  n->edge_traverser (e);
  return e;
}

template <typename T>
inline traversal::relational::edge_base&
operator>> (instance<T>& n, traversal::relational::edge_base& e)
{
  n->edge_traverser (e);
  return e;
}

template <typename T>
inline traversal::node_base&
operator>> (traversal::edge_base& e, instance<T>& n)
{
  e.node_traverser (*n);
  return *n;
}

template <typename T>
inline traversal::relational::node_base&
operator>> (traversal::relational::edge_base& e, instance<T>& n)
{
  e.node_traverser (*n);
  return *n;
}

#endif // ODB_INSTANCE_HXX
