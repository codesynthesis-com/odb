// file      : odb/semantics/elements.ixx
// license   : GNU GPL v3; see accompanying LICENSE file

namespace semantics
{
  // typedefs
  //
  inline typedefs::type_type& typedefs::
  type () const
  {
    return dynamic_cast<type_type&> (named ());
  }
}
