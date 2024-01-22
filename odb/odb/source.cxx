// file      : odb/source.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/common.hxx>
#include <odb/context.hxx>
#include <odb/generate.hxx>
#include <odb/diagnostics.hxx>

using namespace std;

namespace source
{
  struct class_: traversal::class_, virtual context
  {
    class_ ()
        : typedefs_ (false),
          query_columns_type_ (false, false, false),
          view_query_columns_type_ (false)
    {
      *this >> defines_ >> *this;
      *this >> typedefs_ >> *this;
    }

    virtual void
    traverse (type& c)
    {
      class_kind_type ck (class_kind (c));

      if (ck == class_other ||
          (!options.at_once () && class_file (c) != unit.file ()))
        return;

      names (c);

      switch (ck)
      {
      case class_object: traverse_object (c); break;
      case class_view: traverse_view (c); break;
      default: break;
      }
    }

    void
    traverse_object (type&);

    void
    traverse_view (type&);

  private:
    traversal::defines defines_;
    typedefs typedefs_;

    instance<query_columns_type> query_columns_type_;
    instance<view_query_columns_type> view_query_columns_type_;
  };
}

void source::class_::
traverse_object (type& c)
{
  bool poly (polymorphic (c));
  bool abst (abstract (c));
  bool reuse_abst (abst && !poly);

  // The rest only applies to dynamic milti-database support.
  //
  if (!multi_dynamic)
    return;

  os << "// " << class_name (c) << endl
     << "//" << endl
     << endl;

  // query_columns
  //
  if (options.generate_query ())
    query_columns_type_->traverse (c);

  // The rest does not apply to reuse-abstract objects.
  //
  if (reuse_abst)
    return;

  string const& type (class_fq_name (c));
  string traits ("access::object_traits_impl< " + type + ", id_common >");

  os << "const " << traits << "::" << endl
     << "function_table_type*" << endl
     << traits << "::" << endl
     << "function_table[database_count];"
     << endl;
}

void source::class_::
traverse_view (type& c)
{
  // The rest only applies to dynamic milti-database support.
  //
  if (!multi_dynamic)
    return;

  os << "// " << class_name (c) << endl
     << "//" << endl
     << endl;

  if (c.get<size_t> ("object-count") != 0)
    view_query_columns_type_->traverse (c);

  string const& type (class_fq_name (c));
  string traits ("access::view_traits_impl< " + type + ", id_common >");

  os << "const " << traits << "::" << endl
     << "function_table_type*" << endl
     << traits << "::" << endl
     << "function_table[database_count];"
     << endl;
}

namespace source
{
  void
  generate ()
  {
    context ctx;
    ostream& os (ctx.os);

    traversal::unit unit;
    traversal::defines unit_defines;
    typedefs unit_typedefs (false);
    traversal::namespace_ ns;
    class_ c;

    unit >> unit_defines >> ns;
    unit_defines >> c;
    unit >> unit_typedefs >> c;

    traversal::defines ns_defines;
    typedefs ns_typedefs (false);

    ns >> ns_defines >> ns;
    ns_defines >> c;
    ns >> ns_typedefs >> c;

    os << "namespace odb"
       << "{";

    unit.dispatch (ctx.unit);

    os << "}";
  }
}
