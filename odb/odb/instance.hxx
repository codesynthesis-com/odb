// file      : odb/instance.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_INSTANCE_HXX
#define ODB_INSTANCE_HXX

#include <map>
#include <string>
#include <cstddef>  // std::size_t
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
  create (B const& prototype)
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

  typedef B* (*create_func) (B const&);
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
  create (base const& prototype)
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

  instance ()
  {
    base_type prototype;
    x_ = factory_type::create (prototype);
  }

  template <typename A1>
  instance (A1& a1)
  {
    base_type prototype (a1);
    x_ = factory_type::create (prototype);
  }

  template <typename A1>
  instance (A1 const& a1)
  {
    base_type prototype (a1);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2>
  instance (A1& a1, A2& a2)
  {
    base_type prototype (a1, a2);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2>
  instance (A1 const& a1, A2 const& a2)
  {
    base_type prototype (a1, a2);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3>
  instance (A1& a1, A2& a2, A3& a3)
  {
    base_type prototype (a1, a2, a3);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3>
  instance (A1 const& a1, A2 const& a2, A3 const& a3)
  {
    base_type prototype (a1, a2, a3);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3, typename A4>
  instance (A1& a1, A2& a2, A3& a3, A4& a4)
  {
    base_type prototype (a1, a2, a3, a4);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3, typename A4>
  instance (A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4)
  {
    base_type prototype (a1, a2, a3, a4);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5>
  instance (A1& a1, A2& a2, A3& a3, A4& a4, A5& a5)
  {
    base_type prototype (a1, a2, a3, a4, a5);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5>
  instance (A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4,
            A5 const& a5)
  {
    base_type prototype (a1, a2, a3, a4, a5);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
            typename A6>
  instance (A1& a1, A2& a2, A3& a3, A4& a4, A5& a5, A6 a6)
  {
    base_type prototype (a1, a2, a3, a4, a5, a6);
    x_ = factory_type::create (prototype);
  }

  template <typename A1, typename A2, typename A3, typename A4, typename A5,
            typename A6>
  instance (A1 const& a1, A2 const& a2, A3 const& a3, A4 const& a4,
            A5 const& a5, A6 const& a6)
  {
    base_type prototype (a1, a2, a3, a4, a5, a6);
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
