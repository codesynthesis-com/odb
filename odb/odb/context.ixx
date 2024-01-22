// file      : odb/context.ixx
// license   : GNU GPL v3; see accompanying LICENSE file

inline bool user_section::
load_empty () const
{
  return !separate_load () || (total == 0 && !containers && !optimistic ());
}

inline bool user_section::
update_empty () const
{
  return total == inverse + readonly && !readwrite_containers;
}

inline bool user_section::
optimistic () const
{
  if (!context::optimistic (*object))
    return false;
  else
  {
    semantics::class_* poly_root (context::polymorphic (*object));
    return poly_root == 0 || poly_root == object;
  }
}
