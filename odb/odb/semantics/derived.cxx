// file      : odb/semantics/derived.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <sstream>

#include <libcutl/compiler/type-info.hxx>
#include <odb/semantics/derived.hxx>

using namespace std;

namespace semantics
{
  qualifies::
  qualifies ()
      : hint_ (0)
  {
  }

  qualifier::
  qualifier (path const& file,
             size_t line,
             size_t column,
             tree tn,
             bool c,
             bool v,
             bool r)
      : node (file, line, column, tn), c_ (c), v_ (v), r_ (r)
  {
  }

  string qualifier::
  fq_name (names* hint) const
  {
    if (hint != 0 || defined_ != 0)
      return nameable::fq_name (hint);

    // GCC type_as_string() for some reason cannot correctly print names
    // like 'const std::string'. Instead it prints 'const string'.
    //
    type& bt (base_type ());

    // Use the trailing qualifier syntax so that we don't get bogged down
    // in stuff like 'const const foo*'. We also have to handle arrays in
    // a special way since char[16] const is not a legal syntax.
    //
    string q;
    if (c_)
      q += " const";

    if (v_)
      q += " volatile";

    if (r_)
      q += " __restrict";

    hint = qualifies ().hint ();

    if (array* a = dynamic_cast<array*> (&bt))
      return a->fq_name (hint, q);
    else
      return bt.fq_name (hint) + q;
  }

  points::
  points ()
      : hint_ (0)
  {
  }

  pointer::
  pointer (path const& file, size_t line, size_t column, tree tn)
      : node (file, line, column, tn)
  {
  }

  string pointer::
  fq_name (names* hint) const
  {
    if (hint != 0 || defined_ != 0)
      return nameable::fq_name (hint);

    // GCC type_as_string() for some reason cannot correctly print names
    // like 'const std::string*'. Instead it prints 'const string*'.
    //
    string r (base_type ().fq_name (points ().hint ()));
    r += '*';
    return r;
  }

  references::
  references ()
      : hint_ (0)
  {
  }

  reference::
  reference (path const& file, size_t line, size_t column, tree tn)
      : node (file, line, column, tn)
  {
  }

  string reference::
  fq_name (names* hint) const
  {
    if (hint != 0 || defined_ != 0)
      return nameable::fq_name (hint);

    // GCC type_as_string() for some reason cannot correctly print names
    // like 'const std::string&'. Instead it prints 'const string&'.
    //
    string r (base_type ().fq_name (points ().hint ()));
    r += '&';
    return r;
  }

  contains::
  contains ()
      : hint_ (0)
  {
  }

  array::
  array (path const& file,
         size_t line,
         size_t column,
         tree tn,
         unsigned long long size)
      : node (file, line, column, tn), size_ (size)
  {
  }

  string array::
  fq_name (names* hint) const
  {
    // GCC type_as_string() for some reason cannot correctly print names
    // like 'const std::string[123]'. Instead it prints 'const string[123]'.
    //
    string t;
    return fq_name (hint, t);
  }

  string array::
  fq_name (names* hint, string& t) const
  {
    if (hint != 0 || defined_ != 0)
      return nameable::fq_name (hint) + t;

    t += '[';
    ostringstream ostr;
    ostr << size ();
    t += ostr.str ();

    if (size () > 0xFFFFFFFF)
      t += "ULL";
    else if (size () > 2147483647)
      t += "U";

    t += ']';

    type& bt (base_type ());
    hint = contains ().hint ();
    array* a;

    if (hint != 0 || (a = dynamic_cast<array*> (&bt)) == 0)
      return bt.fq_name (hint) + t;
    else
      return a->fq_name (0, t);
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

        // derived_type
        //
        {
          type_info ti (typeid (derived_type));
          ti.add_base (typeid (type));
          insert (ti);
        }

        // qualifies
        //
        {
          type_info ti (typeid (qualifies));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // qualifier
        //
        {
          type_info ti (typeid (qualifier));
          ti.add_base (typeid (derived_type));
          insert (ti);
        }

        // points
        //
        {
          type_info ti (typeid (points));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // pointer
        //
        {
          type_info ti (typeid (pointer));
          ti.add_base (typeid (derived_type));
          insert (ti);
        }

        // references
        //
        {
          type_info ti (typeid (references));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // reference
        //
        {
          type_info ti (typeid (reference));
          ti.add_base (typeid (derived_type));
          insert (ti);
        }

        // contains
        //
        {
          type_info ti (typeid (contains));
          ti.add_base (typeid (edge));
          insert (ti);
        }

        // array
        //
        {
          type_info ti (typeid (array));
          ti.add_base (typeid (derived_type));
          insert (ti);
        }
      }
    } init_;
  }
}
