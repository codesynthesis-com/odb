// file      : odb/semantics/relational/elements.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <libcutl/compiler/type-info.hxx>

#include <odb/semantics/relational/elements.hxx>
#include <odb/semantics/relational/column.hxx>
#include <odb/semantics/relational/primary-key.hxx>

namespace semantics
{
  namespace relational
  {
    string const xmlns = "http://www.codesynthesis.com/xmlns/odb/changelog";

    // duplicate_name
    //
    template <>
    duplicate_name::
    duplicate_name (uscope& s, unameable& o, unameable& d)
        : scope (s), orig (o), dup (d), name (o.name ())
    {
    }

    template <>
    duplicate_name::
    duplicate_name (qscope& s, qnameable& o, qnameable& d)
        : scope (s), orig (o), dup (d), name (o.name ().string ())
    {
    }

    // scope<uname>
    //
    template <>
    void scope<uname>::
    add_edge_left (names_type& e)
    {
      nameable_type& n (e.nameable ());
      name_type const& name (e.name ());

      typename names_map::iterator i (names_map_.find (name));

      if (i == names_map_.end ())
      {
        typename names_list::iterator i;

        // We want the order to be add/alter columns first, then the
        // primary key, then other keys, and finnally drop columns.
        //
        if (n.is_a<column> () ||
            n.is_a<add_column> () ||
            n.is_a<alter_column> ())
        {
          i = names_.insert (first_key_, &e);
        }
        else if (!n.is_a<drop_column> ())
        {
          if (n.is_a<primary_key> ())
            first_key_ = i = names_.insert (
              first_key_ != names_.end () ? first_key_ : first_drop_column_,
              &e);
          else
          {
            i = names_.insert (first_drop_column_, &e);

            if (first_key_ == names_.end ())
              first_key_ = i;
          }
        }
        else
        {
          i = names_.insert (names_.end (), &e);

          if (first_drop_column_ == names_.end ())
            first_drop_column_ = i;
        }

        names_map_[name] = i;
        iterator_map_[&e] = i;
      }
      else
        throw duplicate_name (*this, (*i->second)->nameable (), n);
    }

    template <>
    void scope<uname>::
    remove_edge_left (names_type& e)
    {
      typename names_iterator_map::iterator i (iterator_map_.find (&e));
      assert (i != iterator_map_.end ());

      // If we are removing the first key, then move to the next key (or
      // the end which means there are no keys).
      //
      if (first_key_ == i->second)
        first_key_++;

      // The same for the first drop column.
      //
      if (first_drop_column_ == i->second)
        first_drop_column_++;

      names_.erase (i->second);
      names_map_.erase (e.name ());
      iterator_map_.erase (i);
    }

    // type info
    //
    namespace
    {
      struct init
      {
        init ()
        {
          using compiler::type_info;

          // node
          //
          insert (type_info (typeid (node)));

          // edge
          //
          insert (type_info (typeid (edge)));

          // alters
          //
          {
            type_info ti (typeid (alters));
            ti.add_base (typeid (edge));
            insert (ti);
          }

          // names
          //
          {
            type_info ti (typeid (unames));
            ti.add_base (typeid (edge));
            insert (ti);
          }

          {
            type_info ti (typeid (qnames));
            ti.add_base (typeid (edge));
            insert (ti);
          }

          // nameable
          //
          {
            type_info ti (typeid (unameable));
            ti.add_base (typeid (node));
            insert (ti);
          }

          {
            type_info ti (typeid (qnameable));
            ti.add_base (typeid (node));
            insert (ti);
          }

          // scope
          //
          {
            type_info ti (typeid (uscope));
            ti.add_base (typeid (node));
            insert (ti);
          }

          {
            type_info ti (typeid (qscope));
            ti.add_base (typeid (node));
            insert (ti);
          }
        }
      } init_;
    }
  }
}
