// file      : odb/relational/common-query.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_COMMON_QUERY_HXX
#define ODB_RELATIONAL_COMMON_QUERY_HXX

#include <odb/relational/common.hxx>

namespace relational
{
  //
  //
  struct query_alias_traits: ::query_alias_traits, virtual context
  {
    typedef query_alias_traits base_impl;

    query_alias_traits (base const& x): base (x) {}

    virtual void
    generate_decl_body ();

    virtual void
    generate_def (semantics::data_member&, semantics::class_&);

    virtual void
    generate_def (string const& tag, semantics::class_&, string const& alias);
  };

  //
  //
  struct query_columns_base: ::query_columns_base, virtual context
  {
    typedef query_columns_base base_impl;

    query_columns_base (base const& x): base (x) {const_ = "const ";}
  };

  //
  //
  struct query_columns: ::query_columns, virtual context
  {
    typedef query_columns base_impl;

    query_columns (base const& x): base (x) {const_ = "const ";}

    virtual string
    database_type_id (semantics::data_member&) = 0;

    virtual void
    column_ctor (string const& type, string const& name, string const& base);

    virtual void
    column_ctor_args_extra (semantics::data_member&);

    virtual void
    column_common (semantics::data_member&,
                   string const& type,
                   string const& column,
                   string const& suffix);
  };
}

#endif // ODB_RELATIONAL_COMMON_QUERY_HXX
