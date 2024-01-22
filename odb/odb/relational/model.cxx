// file      : odb/relational/model.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/diagnostics.hxx>

#include <odb/relational/model.hxx>
#include <odb/relational/generate.hxx>

using namespace std;

namespace relational
{
  namespace model
  {
    // object_columns
    //
    string object_columns::
    default_ (semantics::data_member& m)
    {
      default_value* dv (0);

      semantics::type& t (utype (m));

      if (m.count ("default"))
        dv = &m.get<default_value> ("default");
      else if (t.count ("default"))
        dv = &t.get<default_value> ("default");
      else
        return ""; // No default value for this column.

      switch (dv->kind)
      {
      case default_value::reset:
        {
          return ""; // No default value.
        }
      case default_value::null:
        {
          return default_null (m);
        }
      case default_value::boolean:
        {
          return default_bool (m, dv->literal == "true");
        }
      case default_value::integer:
        {
          return default_integer (m, dv->int_value, dv->literal == "-");
        }
      case default_value::floating:
        {
          return default_float (m, dv->float_value);
        }
      case default_value::string:
        {
          return default_string (m, dv->literal);
        }
      case default_value::enumerator:
        {
          return default_enum (m, dv->enum_value, dv->literal);
        }
      }

      return "";
    }

    cutl::shared_ptr<sema_rel::model>
    generate ()
    {
      context ctx;
      cutl::shared_ptr<sema_rel::model> m (
        new (shared) sema_rel::model (
          ctx.versioned () ? ctx.version ().current : 0));
      m->set ("deleted-map", deleted_table_map ());

      traversal::unit unit;
      traversal::defines unit_defines;
      typedefs unit_typedefs (false);
      traversal::namespace_ ns;
      instance<class_> c (*m);

      unit >> unit_defines >> ns;
      unit_defines >> c;
      unit >> unit_typedefs >> c;

      traversal::defines ns_defines;
      typedefs ns_typedefs (false);

      ns >> ns_defines >> ns;
      ns_defines >> c;
      ns >> ns_typedefs >> c;

      try
      {
        unit.dispatch (ctx.unit);
      }
      catch (sema_rel::duplicate_name const& e)
      {
        location const& o (e.orig.get<location> ("cxx-location"));
        location const& d (e.dup.get<location> ("cxx-location"));

        error (d) << e.dup.kind () << " name '" << e.name << "' conflicts "
                  << "with an already defined " << e.orig.kind () << " name"
                  << endl;

        info (o) << "conflicting " << e.orig.kind () << " is defined here"
                 << endl;

        if (e.dup.kind () == "index")
          info (d) << "use #pragma db index to change its name" << endl;
        else if (e.dup.kind () == "table")
          info (d) << "use #pragma db table to change its name" << endl;
        else
          info (d) << "use #pragma db column to change its name" << endl;

        throw operation_failed ();
      }

      return m;
    }
  }
}
