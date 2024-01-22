// file      : odb/semantics/relational/elements.txx
// license   : GNU GPL v3; see accompanying LICENSE file

namespace semantics
{
  namespace relational
  {
    // nameable
    //
    template <typename N>
    typename nameable<N>::parser_map nameable<N>::parser_map_;

    template <typename N>
    template <typename T>
    void nameable<N>::
    parser_impl (xml::parser& p, scope_type& s, graph& g)
    {
      name_type n (p.attribute ("name", name_type ()));
      T& x (g.new_node<T> (p, s, g));
      g.new_edge<names_type> (s, x, n);
    }

    template <typename N>
    nameable<N>::
    nameable (nameable const& n, graph&)
        : id_ (n.id_), named_ (0)
    {
    }

    template <typename N>
    nameable<N>::
    nameable (xml::parser&, graph&)
      // : id_ (p.attribute<string> ("id"))
        : named_ (0)
    {
      // The name attribute is handled in parser_impl().
    }

    template <typename N>
    void nameable<N>::
    serialize_attributes (xml::serializer& s) const
    {
      // Omit empty names (e.g., a primary key).
      //
      name_type const& n (name ());
      if (!n.empty ())
        s.attribute ("name", n);

      //s.attribute ("id", id_);
    }

    // scope
    //

    template <typename N>
    template <typename T, typename S>
    T* scope<N>::
    lookup (name_type const& name)
    {
      if (T* r = find<T> (name))
        return r;

      if (scope* b = base ())
      {
        if (find<S> (name) == 0)
          return b->lookup<T, S> (name);
      }

      return 0;
    }

    template <typename N>
    template <typename T>
    T* scope<N>::
    find (name_type const& name)
    {
      typename names_map::iterator i (names_map_.find (name));
      return i != names_map_.end ()
        ? dynamic_cast<T*> (&(*i->second)->nameable ())
        : 0;
    }

    template <typename N>
    typename scope<N>::names_iterator scope<N>::
    find (name_type const& name)
    {
      typename names_map::iterator i (names_map_.find (name));

      if (i == names_map_.end ())
        return names_.end ();
      else
        return i->second;
    }

    template <typename N>
    typename scope<N>::names_const_iterator scope<N>::
    find (name_type const& name) const
    {
      typename names_map::const_iterator i (names_map_.find (name));

      if (i == names_map_.end ())
        return names_.end ();
      else
        return names_const_iterator (i->second);
    }

    template <typename N>
    typename scope<N>::names_iterator scope<N>::
    find (names_type const& e)
    {
      typename names_iterator_map::iterator i (iterator_map_.find (&e));
      return i != iterator_map_.end () ? i->second : names_.end ();
    }

    template <typename N>
    typename scope<N>::names_const_iterator scope<N>::
    find (names_type const& e) const
    {
      typename names_iterator_map::const_iterator i (iterator_map_.find (&e));
      return i != iterator_map_.end () ? i->second : names_.end ();
    }

    template <typename N>
    scope<N>::
    scope (scope const& s, scope* base, graph& g)
        : first_key_ (names_.end ()),
          first_drop_column_ (names_.end ()),
          alters_ (0)
    {
      // Set the alters edge for lookup.
      //
      if (base != 0)
        g.new_edge<alters> (*this, *base);

      for (names_const_iterator i (s.names_begin ());
           i != s.names_end (); ++i)
      {
        nameable_type& n (i->nameable ().clone (*this, g));
        g.new_edge<names_type> (*this, n, i->name ());
      }
    }

    template <typename N>
    scope<N>::
    scope (xml::parser& p, scope* base, graph& g)
        : first_key_ (names_.end ()),
          first_drop_column_ (names_.end ()),
          alters_ (0)
    {
      // Set the alters edge for lookup.
      //
      if (base != 0)
        g.new_edge<alters> (*this, *base);

      using namespace xml;
      p.content (content::complex);

      for (parser::event_type e (p.peek ());
           e == parser::start_element;
           e = p.peek ())
      {
        typename nameable_type::parser_map::iterator i (
          nameable_type::parser_map_.find (p.name ()));

        if (p.namespace_ () != xmlns || i == nameable_type::parser_map_.end ())
          break; // Not one of our elements.

        p.next ();
        i->second (p, *this, g);
        p.next_expect (parser::end_element);
      }
    }

    template <typename N>
    void scope<N>::
    serialize_content (xml::serializer& s) const
    {
      for (names_const_iterator i (names_begin ()); i != names_end (); ++i)
        i->nameable ().serialize (s);
    }

    class column;
    class primary_key;

    template <typename N>
    void scope<N>::
    add_edge_left (names_type& e)
    {
      name_type const& name (e.name ());

      typename names_map::iterator i (names_map_.find (name));

      if (i == names_map_.end ())
      {
        typename names_list::iterator i (names_.insert (names_.end (), &e));
        names_map_[name] = i;
        iterator_map_[&e] = i;
      }
      else
        throw duplicate_name (*this, (*i->second)->nameable (), e.nameable ());
    }

    template <typename N>
    void scope<N>::
    remove_edge_left (names_type& e)
    {
      typename names_iterator_map::iterator i (iterator_map_.find (&e));
      assert (i != iterator_map_.end ());

      names_.erase (i->second);
      names_map_.erase (e.name ());
      iterator_map_.erase (i);
    }
  }
}
