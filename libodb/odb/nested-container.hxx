// file      : odb/nested-container.hxx
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_NESTED_CONTAINER_HXX
#define ODB_NESTED_CONTAINER_HXX

#include <odb/pre.hxx>

#include <cstddef> // size_t

#include <odb/forward.hxx>
#include <odb/details/config.hxx> // ODB_CXX11

#ifndef ODB_CXX11
#  error nested container support is only available in C++11
#endif

namespace odb
{
  // Nested container emulation support for ODB.
  //
  // In a nutshell, the idea is to represent a nested container, for example,
  // vector<vector<V>>, as map<nested_key, V> where nested_key is a composite
  // key consisting of the outer and inner container indexes.
  //
  // See container-nested in odb-examples for some examples.
  //
  // Note that with this approach the empty trailing entries of the outer
  // container will not be added on load. It is assumed that the user handles
  // that on their own, for example, by pre-loading the outer container entry
  // members if there are any.
  //
  // Also note that the outer key in the inner container should strictly
  // speaking be a foreign key pointing to the key of the outer container. The
  // only way to achieve this currently is to manually add the constraint via
  // ALTER TABLE ADD CONSTRAINT. Note, however, that as long as we only modify
  // these tables via the ODB container interface, not having the foreign key
  // (and not having ON DELETE CASCADE) should be harmless (since we have a
  // foreign key pointing to the object id).

  // Map key that is used to emulate 1-level nested container mapping (for
  // example, vector<vector<V>>). Template parameter IC is a tag that allows
  // us to distinguish keys for unrelated containers in order to assign column
  // names, etc. Use the inner container type (for example, vector<V>) for IC.
  //
  template <typename IC,
            typename O = std::size_t,
            typename I = std::size_t>
  struct nested_key
  {
    using outer_type = O;
    using inner_type = I;

    outer_type outer;
    inner_type inner;

    nested_key () = default;
    nested_key (outer_type o, inner_type i): outer (o), inner (i) {}

    bool
    operator< (const nested_key& v) const
    {
      return outer < v.outer || (outer == v.outer && inner < v.inner);
    }
  };

  // Map key that is used to emulate 2-level nested container mapping (for
  // example, vector<vector<vector<V>>>>). Use the middle container type for
  // MC (for example, vector<vector<V>>).
  //
  template <typename MC,
            typename O = std::size_t,
            typename M = std::size_t,
            typename I = std::size_t>
  struct nested2_key
  {
    using outer_type  = O;
    using middle_type = M;
    using inner_type  = I;

    outer_type  outer;
    middle_type middle;
    inner_type  inner;

    nested2_key () = default;
    nested2_key (outer_type o, middle_type m, inner_type i)
        : outer (o), middle (m), inner (i) {}

    bool
    operator< (const nested2_key& v) const
    {
      return outer  != v.outer  ? outer  < v.outer  :
             middle != v.middle ? middle < v.middle :
                                  inner  < v.inner  ;
    }
  };
}

#include <map>
#include <utility>     // move(), declval()
#include <cassert>
#include <type_traits> // remove_reference, enable_if, is_same

namespace odb
{
  template <typename C>
  struct nested1_type:
    std::remove_reference<decltype (std::declval<C> ()[0])> {};

  template <typename C>
  struct nested2_type:
    std::remove_reference<decltype (std::declval<C> ()[0][0])> {};

  template <typename C>
  struct nested3_type:
    std::remove_reference<decltype (std::declval<C> ()[0][0][0])> {};

  // 1-level nesting of std::vector-like containers.
  //
  template <typename OC, // For example, OC = vector<vector<V>>.
            typename K = nested_key<typename nested1_type<OC>::type>>
  typename std::enable_if<std::is_same<typename OC::value_type,
                                       typename nested1_type<OC>::type>::value,
                          std::map<K, typename nested2_type<OC>::type>>::type
  nested_get (const OC& oc)
  {
    using namespace std;

    using IC = typename nested1_type<OC>::type;
    using V = typename nested2_type<OC>::type;

    map<K, V> r;
    for (size_t o (0); o != oc.size (); ++o)
    {
      const IC& ic (oc[o]);
      for (size_t i (0); i != ic.size (); ++i)
        r.emplace (K (o, i), ic[i]);
    }
    return r;
  }

  template <typename OC, typename K, typename V>
  typename std::enable_if<
    std::is_same<typename OC::value_type,
                 typename nested1_type<OC>::type>::value>::type
  nested_set (OC& oc, std::map<K, V>&& r)
  {
    using namespace std;

    // Cleanup the nested containers before (re-)loading.
    //
    // Note that the entries of the outer container may potentially be value
    // types derived from a container type. This value type may potentially
    // override the container's clear() function (its signature is quite
    // common). Thus, to clean up the nested containers let's use their
    // erase(iterator,iterator) function, which is unlikely to be overridden
    // by the value type.
    //
    for (auto& c: oc)
      c.erase (c.begin (), c.end ());

    for (auto& p: r)
    {
      size_t o (p.first.outer);
      V& v (p.second);

      if (o >= oc.size ())
        oc.resize (o + 1);

      assert (p.first.inner == oc[o].size ());

      oc[o].push_back (move (v));
    }
  }

  // 1-level nesting in std::vector-like containers of value types which
  // contain std::vector-like containers.
  //
  template <typename OC, // For example, OC = vector<T1> (class T1{ IC m; }).
            typename IC, // For example, IC = vector<T2>.
            typename K = nested_key<IC>>
  typename std::enable_if<std::is_same<typename OC::value_type,
                                       typename nested1_type<OC>::type>::value,
                          std::map<K, typename nested1_type<IC>::type>>::type
  nested_get (const OC& oc, IC OC::value_type::* m)
  {
    using namespace std;

    using V = typename nested1_type<IC>::type;

    map<K, V> r;
    for (size_t o (0); o != oc.size (); ++o)
    {
      const IC& ic (oc[o].*m);
      for (size_t i (0); i != ic.size (); ++i)
        r.emplace (K (o, i), ic[i]);
    }
    return r;
  }

  template <typename OC, typename IC, typename K, typename V>
  typename std::enable_if<
    std::is_same<typename OC::value_type,
                 typename nested1_type<OC>::type>::value>::type
  nested_set (OC& oc, IC OC::value_type::* m, std::map<K, V>&& r)
  {
    using namespace std;

    for (auto& o: oc)
    {
      auto& c (o.*m);
      c.erase (c.begin (), c.end ());
    }

    for (auto& p: r)
    {
      size_t o (p.first.outer);
      V& v (p.second);

      if (o >= oc.size ())
        oc.resize (o + 1);

      IC& ic (oc[o].*m);

      assert (p.first.inner == ic.size ());

      ic.push_back (move (v));
    }
  }

  // 1-level nesting in std::map-like containers of std::vector-like
  // containers.
  //
  template <typename OC, // For example, OC = map<K,vector<V>>.
            typename K = nested_key<typename OC::mapped_type,
                                    typename OC::key_type>>
  typename std::enable_if<
    std::is_same<typename OC::mapped_type,
                 typename OC::value_type::second_type>::value,
    std::map<K, typename nested1_type<typename OC::mapped_type>::type>>::type
  nested_get (const OC& oc)
  {
    using namespace std;

    using IC = typename OC::mapped_type;
    using V = typename nested1_type<IC>::type;

    map<K, V> r;
    for (const auto& p: oc)
    {
      const IC& ic (p.second);
      for (size_t i (0); i != ic.size (); ++i)
        r.emplace (K (p.first, i), ic[i]);
    }
    return r;
  }

  template <typename OC, typename K, typename V>
  typename std::enable_if<
    std::is_same<typename OC::mapped_type,
                 typename OC::value_type::second_type>::value>::type
  nested_set (OC& oc, std::map<K, V>&& r)
  {
    using namespace std;

    for (auto& p: oc)
    {
      auto& c (p.second);
      c.erase (c.begin (), c.end ());
    }

    for (auto& p: r)
    {
      const auto& o (p.first.outer);
      V& v (p.second);

      assert (p.first.inner == oc[o].size ());

      oc[o].push_back (move (v));
    }
  }

  // 2-level nesting of std::vector-like containers.
  //
  template <typename OC, // For example, OC = vector<vector<vector<V>>>.
            typename K = nested2_key<typename nested1_type<OC>::type>>
  std::map<K, typename nested3_type<OC>::type>
  nested2_get (const OC& oc)
  {
    using namespace std;

    using MC = typename nested1_type<OC>::type;
    using IC = typename nested2_type<OC>::type;
    using V = typename nested3_type<OC>::type;

    map<K, V> r;
    for (size_t o (0); o != oc.size (); ++o)
    {
      const MC& mc (oc[o]);
      for (size_t m (0); m != mc.size (); ++m)
      {
        const IC& ic (mc[m]);
        for (size_t i (0); i != ic.size (); ++i)
          r.emplace (K (o, m, i), ic[i]);
      }
    }
    return r;
  }

  template <typename OC, typename K, typename V>
  void
  nested2_set (OC& oc, std::map<K, V>&& r)
  {
    using namespace std;

    for (auto& o: oc)
    {
      for (auto m: o)
        m.erase (m.begin (), m.end ());
    }

    for (auto& p: r)
    {
      size_t o (p.first.outer);
      size_t m (p.first.middle);
      V& v (p.second);

      if (o >= oc.size ())
        oc.resize (o + 1);

      auto& mc (oc[o]);

      if (m >= mc.size ())
        mc.resize (m + 1);

      assert (p.first.inner == mc[m].size ());

      mc[m].push_back (move (v));
    }
  }
}

#include <odb/post.hxx>

#endif // ODB_NESTED_CONTAINER_HXX
