// file      : odb/common.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/common.hxx>

using namespace std;

//
// object_members_base
//

void object_members_base::
traverse_simple (semantics::data_member&)
{
}

void object_members_base::
traverse_pointer (semantics::data_member& m, semantics::class_& c)
{
  if (!view_member (m)) // Not really "as if" pointed-to id member.
    traverse_member (m, utype (*id_member (c)));
}

void object_members_base::
traverse_composite (semantics::data_member*, semantics::class_& c)
{
  inherits (c);
  names (c);
}

void object_members_base::
traverse_composite_wrapper (semantics::data_member* m,
                            semantics::class_& c,
                            semantics::type*)
{
  traverse_composite (m, c);
}

void object_members_base::
traverse_container (semantics::data_member&, semantics::type&)
{
}

void object_members_base::
traverse_object (semantics::class_& c)
{
  inherits (c);
  names (c);
}

void object_members_base::
traverse_view (semantics::class_& c)
{
  // A view has no bases.
  //
  names (c);
}

void object_members_base::
traverse (semantics::class_& c)
{
  class_kind_type k (class_kind (c));

  if (k == class_other)
  {
    // Ignore transient bases.
    //
    return;
  }
  else if (k == class_composite)
  {
    if (member_scope_.empty ())
      member_scope_.push_back (class_inheritance_chain ());

    member_scope_.back ().push_back (&c);

    traverse_composite_wrapper (0, c, 0);

    member_scope_.back ().pop_back ();

    if (member_scope_.back ().empty ())
      member_scope_.pop_back ();

    return;
  }

  if (top_level_)
    top_level_ = false;
  else
  {
    // Unless requested otherwise, don't go into bases if we are a derived
    // type in a polymorphic hierarchy.
    //
    if (!traverse_poly_base_ && polymorphic (c))
      return;
  }

  if (context::top_object == 0)
    context::top_object = &c;

  semantics::class_* prev (context::cur_object);
  context::cur_object = &c;

  if (member_scope_.empty ())
    member_scope_.push_back (class_inheritance_chain ());

  member_scope_.back ().push_back (&c);

  if (build_table_prefix_)
  {
    // Don't reset the table prefix if we are traversing a base.
    //
    bool tb (false);

    if (table_prefix_.level == 0)
    {
      table_prefix_ = table_prefix (c);
      tb = true;
    }

    if (k == class_object)
      traverse_object (c);
    else
      traverse_view (c);

    if (tb)
      table_prefix_ = table_prefix ();
  }
  else
  {
    if (k == class_object)
      traverse_object (c);
    else
      traverse_view (c);
  }

  member_scope_.back ().pop_back ();

  if (member_scope_.back ().empty ())
    member_scope_.pop_back ();

  context::cur_object = prev;

  if (prev == 0)
    context::top_object = 0;
}

void object_members_base::
traverse_member (semantics::data_member& m, semantics::type& t)
{
  if (semantics::class_* comp = context::composite_wrapper (t))
  {
    member_scope_.push_back (class_inheritance_chain ());
    member_scope_.back ().push_back (comp);

    table_prefix old_table_prefix;
    string old_flat_prefix, old_member_prefix;

    if (build_flat_prefix_)
    {
      old_flat_prefix = flat_prefix_;
      flat_prefix_ += public_name (m);
      flat_prefix_ += '_';
    }

    if (build_member_prefix_)
    {
      old_member_prefix = member_prefix_;
      member_prefix_ += m.name ();
      member_prefix_ += '.';
    }

    if (build_table_prefix_)
    {
      old_table_prefix = table_prefix_;
      table_prefix_.append (m);
    }

    traverse_composite_wrapper (&m, *comp, (wrapper (t) ? &t : 0));

    if (build_table_prefix_)
      table_prefix_ = old_table_prefix;

    if (build_flat_prefix_)
      flat_prefix_ = old_flat_prefix;

    if (build_member_prefix_)
      member_prefix_ = old_member_prefix;

    member_scope_.pop_back ();
  }
  else
    traverse_simple (m);
}

bool object_members_base::
section_test (data_member_path const& mp)
{
  // By default ignore members from the wrong section.
  //
  return section_ == 0 || *section_ == section (mp);
}

void object_members_base::member::
traverse (semantics::data_member& m)
{
  if (transient (m))
    return;

  om_.member_path_.push_back (&m);

  // Ignore members from the wrong section.
  //
  if (om_.section_test (om_.member_path_))
  {
    semantics::type& t (utype (m));

    if (semantics::type* c = context::container (m))
      om_.traverse_container (m, *c);
    else if (semantics::class_* c = object_pointer (t))
      om_.traverse_pointer (m, *c);
    else
      om_.traverse_member (m, t);
  }

  om_.member_path_.pop_back ();
}

//
// object_columns_base
//

void object_columns_base::
flush ()
{
}

bool object_columns_base::
traverse_column (semantics::data_member&, string const&, bool)
{
  return false;
}

void object_columns_base::
traverse_pointer (semantics::data_member& m, semantics::class_& c)
{
  if (!view_member (m)) // Not really "as if" pointed-to id column.
    traverse_member (m, utype (*id_member (c)));
}

void object_columns_base::
traverse_points_to (semantics::data_member& m, semantics::class_&)
{
  traverse_member (m, utype (m));
}

void object_columns_base::
traverse_composite (semantics::data_member*, semantics::class_& c)
{
  inherits (c);
  names (c);
}

void object_columns_base::
traverse_object (semantics::class_& c)
{
  inherits (c);
  names (c);
}

void object_columns_base::
traverse_view (semantics::class_& c)
{
  // A view has no bases.
  //
  names (c);
}

void object_columns_base::
traverse_pre (semantics::nameable&)
{
}

void object_columns_base::
traverse_post (semantics::nameable&)
{
}

void object_columns_base::
traverse (semantics::data_member& m)
{
  traverse_pre (m);

  semantics::type& t (utype (m));
  semantics::class_* c (object_pointer (t));
  semantics::type* rt (c == 0 ? &t : &utype (*id_member (*c)));

  root_ = &m;

  // It would seem natural to add m to member_path_ so that we don't
  // have these two cases. However, for member path to work correctly
  // with readonly() we also have to have corresponding member_scope,
  // which is a whole different level of complexity.
  //
  root_id_ = member_path_.empty ()
    ? context::id (m)
    : context::id (member_path_) != 0;
  root_op_ = (c != 0);
  root_null_ = context::null (m);


  if (root_op_)
    traverse_pointer (m, *c);
  else
    traverse_member (m, *rt);

  if (!first_ && composite_wrapper (*rt))
    flush ();

  root_ = 0;

  traverse_post (m);
}

void object_columns_base::
traverse (semantics::data_member& m,
          semantics::type& t,
          std::string const& kp,
          std::string const& dn,
          semantics::class_* to)
{
  traverse_pre (m);

  semantics::class_* oto (context::top_object);

  if (to != 0)
    context::top_object = to;

  semantics::class_* c (object_pointer (t));
  semantics::type* rt (c == 0 ? &t : &utype (*id_member (*c)));

  root_ = &m;
  root_id_ = (kp == "id");
  root_op_ = (c != 0);
  root_null_ = context::null (m, kp);

  key_prefix_ = kp;
  default_name_ = dn;

  if (root_op_)
    traverse_pointer (m, *c);
  else
    traverse_member (m, *rt);

  key_prefix_.clear ();
  default_name_.clear ();

  if (!first_ && composite_wrapper (*rt))
    flush ();

  root_ = 0;
  context::top_object = oto;

  traverse_post (m);
}

void object_columns_base::
traverse (semantics::class_& c)
{
  class_kind_type k (class_kind (c));

  // Ignore transient bases.
  //
  if (k == class_other)
    return;

  bool f (top_level_);

  if (top_level_)
  {
    traverse_pre (c);
    top_level_ = false;
  }
  else
  {
    // Unless requested otherwise, don't go into bases if we are a derived
    // type in a polymorphic hierarchy.
    //
    if (!traverse_poly_base_ && polymorphic (c))
      return;
  }

  semantics::class_* prev (0);
  if (k == class_object || k == class_view)
  {
    if (context::top_object == 0)
      context::top_object = &c;

    prev = context::cur_object;
    context::cur_object = &c;
  }

  if (member_scope_.empty ())
    member_scope_.push_back (class_inheritance_chain ());

  member_scope_.back ().push_back (&c);

  if (k == class_object)
    traverse_object (c);
  else if (k == class_view)
    traverse_view (c);
  else if (k == class_composite)
    traverse_composite (0, c);

  member_scope_.back ().pop_back ();

  if (member_scope_.back ().empty ())
    member_scope_.pop_back ();

  if (k == class_object || k == class_view)
  {
    context::cur_object = prev;

    if (prev == 0)
      context::top_object = 0;
  }

  if (f)
  {
    if (!first_)
      flush ();

    traverse_post (c);
  }
}

void object_columns_base::
traverse_member (semantics::data_member& m, semantics::type& t)
{
  if (semantics::class_* comp = composite_wrapper (t))
  {
    member_scope_.push_back (class_inheritance_chain ());
    member_scope_.back ().push_back (comp);

    column_prefix old_prefix (column_prefix_);
    column_prefix_.append (m, key_prefix_, default_name_);

    // Save and clear the key prefix and default name.
    //
    string old_kp, old_dn;
    old_kp.swap (key_prefix_);
    old_dn.swap (default_name_);

    traverse_composite (&m, *comp);

    old_kp.swap (key_prefix_);
    old_dn.swap (default_name_);

    column_prefix_ = old_prefix;
    member_scope_.pop_back ();
  }
  else
  {
    string name (column_name (m, key_prefix_, default_name_, column_prefix_));

    if (traverse_column (m, name, first_))
    {
      if (first_)
        first_ = false;
    }
  }
}

bool object_columns_base::
section_test (data_member_path const& mp)
{
  // By default ignore members from the wrong section.
  //
  return section_ == 0 || *section_ == section (mp);
}

void object_columns_base::member::
traverse (semantics::data_member& m)
{
  if (transient (m))
    return;

  // Container gets its own table, so nothing to do here.
  //
  if (container (m))
    return;

  oc_.member_path_.push_back (&m);

  if (oc_.section_test (oc_.member_path_))
  {
    using semantics::class_;
    semantics::type& t (utype (m));

    if (class_* c = object_pointer (t))
      oc_.traverse_pointer (m, *c);
    else if (class_* c = points_to (m))
      oc_.traverse_points_to (m, *c);
    else
      oc_.traverse_member (m, t);
  }

  oc_.member_path_.pop_back ();
}

//
// object_columns_list
//

void object_columns_list::
traverse_pointer (semantics::data_member& m, semantics::class_& c)
{
  // Ignore inverse object pointers.
  //
  if (!ignore_inverse_ || !inverse (m, key_prefix_))
    object_columns_base::traverse_pointer (m, c);
}

bool object_columns_list::
traverse_column (semantics::data_member& m, std::string const& name, bool)
{
  columns_.push_back (column (name, column_type (), m));
  return true;
}

//
// typedefs
//

void typedefs::
traverse (semantics::typedefs& t)
{
  if (check (t))
    traversal::typedefs::traverse (t);
}

bool typedefs::
check (semantics::typedefs& t)
{
  // This typedef must be for a class template instantiation.
  //
  using semantics::class_instantiation;
  class_instantiation* ci (dynamic_cast<class_instantiation*> (&t.type ()));

  if (ci == 0)
    return false;

  // It must be an object, view, or composite value.
  //
  if (class_kind (*ci) == class_other)
    return false;

  // This typedef name should be the one that was used in the pragma.
  //
  using semantics::names;
  tree type (ci->get<tree> ("tree-node"));

  names* hint;
  if (ci->count ("tree-hint"))
    hint = ci->get<names*> ("tree-hint");
  else
  {
    hint = unit.find_hint (type);
    ci->set ("tree-hint", hint); // Cache it.
  }

  if (hint != &t)
    return false;

  // And the definition may have to be in the file we are compiling.
  //
  if (!included_)
  {
    if (!options.at_once () && class_file (*ci) != unit.file ())
      return false;
  }

  return true;
}
