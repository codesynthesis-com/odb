// file      : odb/relational/context.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_CONTEXT_HXX
#define ODB_RELATIONAL_CONTEXT_HXX

#include <odb/context.hxx>

#include <odb/semantics/relational.hxx>
#include <odb/traversal/relational.hxx>

namespace relational
{
  namespace sema_rel = semantics::relational;
  namespace trav_rel = traversal::relational;

  enum statement_kind
  {
    statement_select,
    statement_insert,
    statement_update,
    statement_delete,
    statement_where   // WHERE clause.
  };

  // Index.
  //
  struct index
  {
    location_t loc;      // Location of this index definition.
    std::string name;    // If empty, then derive from the member name.
    std::string type;    // E.g., "UNIQUE", etc.
    std::string method;  // E.g., "BTREE", etc.
    std::string options; // Database-specific index options.

    struct member
    {
      location_t loc;        // Location of this member specifier.
      std::string name;      // Member name, e.g., foo_, foo_.bar_.
      data_member_path path; // Member path.
      std::string options;   // Member options, e.g., "ASC", etc.
    };
    typedef std::vector<member> members_type;

    members_type members;
  };

  typedef std::vector<index> indexes;

  // Indexes in the above vector are in location order.
  //
  struct index_comparator
  {
    bool
    operator() (index const& x, index const& y) const
    {
      return x.loc < y.loc;
    }
  };

  // Custom database type mapping.
  //
  struct custom_db_type
  {
    regex type;
    std::string as;
    std::string to;
    std::string from;
    location_t loc;
  };

  typedef std::vector<custom_db_type> custom_db_types;

  class context: public virtual ::context
  {
  public:
    // Return true if an object or value type has members for which
    // the image can grow. If section is not specified, then ignore
    // separately loaded members. Otherwise ignore members that do
    // not belong to the section.
    //
    bool
    grow (semantics::class_&, user_section* = 0);

    // The same for a member's value type.
    //
    bool
    grow (semantics::data_member&);

    bool
    grow (semantics::data_member&,
          semantics::type&,
          const custom_cxx_type*,
          string const& key_prefix);

  public:
    // Quote SQL string.
    //
    string
    quote_string (string const&) const;

    // Quote SQL identifier.
    //
    string
    quote_id (string const&) const;

    string
    quote_id (qname const&) const;

    // Quoted column and table names.
    //
    string
    column_qname (semantics::data_member& m, column_prefix const& cp) const
    {
      return quote_id (column_name (m, cp));
    }

    string
    column_qname (data_member_path const& mp) const
    {
      return quote_id (column_name (mp));
    }

    string
    column_qname (semantics::data_member& m,
                  string const& key_prefix,
                  string const& default_name,
                  column_prefix const& cp) const
    {
      return quote_id (column_name (m, key_prefix, default_name, cp));
    }

    string
    table_qname (semantics::class_& c) const
    {
      return quote_id (table_name (c));
    }

    string
    table_qname (semantics::class_& obj, data_member_path const& mp) const
    {
      return quote_id (table_name (obj, mp));
    }

    string
    table_qname (semantics::data_member& m, table_prefix const& p) const
    {
      return quote_id (table_name (m, p));
    }

  public:
    string
    index_name (qname const& table, string const& base);

    string
    fkey_name (qname const& table, string const& base);

    // Custom database type conversion.
    //
  public:
    string
    convert_to (string const& expr,
                string const& sqlt,
                semantics::data_member& m)
    {
      string const& conv (current ().convert_expr (sqlt, m, true));
      return conv.empty () ? expr : convert (expr, conv);
    }

    string
    convert_from (string const& expr,
                  string const& sqlt,
                  semantics::data_member& m)
    {
      string const& conv (current ().convert_expr (sqlt, m, false));
      return conv.empty () ? expr : convert (expr, conv);
    }

    // These shortcut versions should only be used on special members
    // (e.g., auto id, version, etc) since they may not determine the
    // proper SQL type in other cases (prefixes, composite ids, etc).
    //
    string
    convert_to (string const& expr, semantics::data_member& m)
    {
      return convert_to (expr, column_type (m), m);
    }

    string
    convert_from (string const& expr, semantics::data_member& m)
    {
      return convert_from (expr, column_type (m), m);
    }

    // Return the conversion expression itself.
    //
    string const&
    convert_to_expr (string const& sqlt, semantics::data_member& m)
    {
      return current ().convert_expr (sqlt, m, true);
    }

  protected:
    virtual string const&
    convert_expr (string const& sqlt, semantics::data_member&, bool to);

    string
    convert (string const& expr, string const& conv);

  protected:
    // The default implementation returns false.
    //
    virtual bool
    grow_impl (semantics::class_&, user_section*);

    virtual bool
    grow_impl (semantics::data_member&);

    virtual bool
    grow_impl (semantics::data_member&,
               semantics::type&,
               const custom_cxx_type*,
               string const&);

    // The default implementation uses the ISO quoting ('') and
    // escapes singe quotes inside the string by double-quoting
    // (' -> ''). Some (most?) database systems support escape
    // sequences. We may want to provide utilize that to support
    // things like \n, \t, etc.
    //
    virtual string
    quote_string_impl (string const&) const;

    // The default implementation uses the ISO quoting ("").
    //
    virtual string
    quote_id_impl (qname const&) const;

  public:
    virtual
    ~context ();
    context ();

    static context&
    current ()
    {
      return *current_;
    }

  protected:
    struct data;
    typedef context base_context;

    context (data*, sema_rel::model*);

  private:
    static context* current_;

  protected:
    struct data: root_context::data
    {
      data (std::ostream& os): root_context::data (os) {}

      string bind_vector_;
      string truncated_vector_;
    };
    data* data_;

  public:
    sema_rel::model* model;

    bool generate_grow;
    bool need_alias_as;
    bool insert_send_auto_id;
    bool delay_freeing_statement_result;
    bool need_image_clone;
    bool generate_bulk;

    bool global_index;
    bool global_fkey;

    string const& bind_vector;
    string const& truncated_vector;
  };
}

#include <odb/relational/context.ixx>

#endif // ODB_RELATIONAL_CONTEXT_HXX
