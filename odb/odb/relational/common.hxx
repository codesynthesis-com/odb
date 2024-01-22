// file      : odb/relational/common.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_COMMON_HXX
#define ODB_RELATIONAL_COMMON_HXX

#include <set>
#include <cassert>

#include <odb/common.hxx>
#include <odb/relational/context.hxx>

namespace relational
{
  struct member_base: traversal::data_member, virtual context
  {
    typedef member_base base;

    member_base (semantics::type* type,
                 const custom_cxx_type* ct,
                 string const& fq_type,
                 string const& key_prefix,
                 object_section* section = 0)
        : type_override_ (type),
          custom_override_ (ct),
          fq_type_override_ (fq_type),
          key_prefix_ (key_prefix),
          section_ (section),
          top_level_ (false)
    {
    }

    member_base (string const& var,
                 semantics::type* type,
                 const custom_cxx_type* ct,
                 string const& fq_type,
                 string const& key_prefix,
                 object_section* section = 0)
        : var_override_ (var),
          type_override_ (type),
          custom_override_ (ct),
          fq_type_override_ (fq_type),
          key_prefix_ (key_prefix),
          section_ (section),
          top_level_ (false)
    {
    }

  protected:
    // For virtual inheritance only. Should not be actually called.
    //
    member_base ();

  protected:
    string var_override_;
    semantics::type* type_override_;
    const custom_cxx_type* custom_override_;
    string fq_type_override_;
    string key_prefix_;
    object_section* section_;

    // True during the top-level call of pre() and post() below. Note that
    // call via another tarverser (e.g., for a class) is not considered top-
    // level.
    //
    bool top_level_;
  };

  // Template argument is the database SQL type (sql_type).
  //
  template <typename T>
  struct member_base_impl: virtual member_base
  {
    typedef member_base_impl base_impl;

    member_base_impl (base const& x): base (x) {}

  protected:
    member_base_impl () {}

  public:
    virtual T const&
    member_sql_type (semantics::data_member&) = 0;

    void
    traverse (semantics::data_member& m, bool top_level)
    {
      top_level_ = top_level;
      traverse (m);
      top_level_ = false;
    }

    struct member_info
    {
      semantics::data_member& m; // Member.
      semantics::type& t;        // Cvr-unqualified member C++ type, note
                                 // that m.type () may not be the same as t.
      const custom_cxx_type* ct; // Translation used for t, if any.
      semantics::class_* ptr;    // Pointed-to object if m is an object
                                 // pointer. In this case t is the id type
                                 // while fq_type_ is the pointer fq-type.
      semantics::type* wrapper;  // Wrapper type if member is a composite or
                                 // container wrapper, also cvr-unqualified.
                                 // In this case t is the wrapped type.
      bool cq;                   // True if the original (wrapper) type
                                 // is const-qualified.
      T const* st;               // Member SQL type (only simple values).
      string& var;               // Member variable name with trailing '_'.

      // C++ type fq-name.
      //
      string
      fq_type (bool unwrap = true) const
      {
        semantics::names* hint;

        if (wrapper != 0 && unwrap)
        {
          // Use the hint from the wrapper unless the wrapped type
          // is qualified.
          //
          hint = wrapper->get<semantics::names*> ("wrapper-hint");
          utype (*context::wrapper (*wrapper), hint);
          return t.fq_name (hint);
        }

        // Use the original type from 'm' instead of 't' since the hint may
        // be invalid for a different type. Plus, if a type is overriden,
        // then the fq_type must be as well.
        //
        if (ptr != 0)
        {
          semantics::type& t (utype (*id_member (*ptr), hint));
          return t.fq_name (hint);
        }
        else if (fq_type_.empty ())
        {
          semantics::type& t (utype (m, hint));
          return t.fq_name (hint);
        }
        else
          // If we are translated, then fq_type_ contains the original type.
          //
          return ct == 0 ? fq_type_ : t.fq_name (ct->as_hint);
      }

      string
      ptr_fq_type () const
      {
        assert (ptr != 0);

        if (fq_type_.empty ())
        {
          // If type is overridden so should fq_type so it is safe to
          // get the type from the member.
          //
          semantics::names* hint;
          semantics::type& t (utype (m, hint));
          return t.fq_name (hint);
        }
        else
          return fq_type_;
      }

      string const& fq_type_;

      member_info (semantics::data_member& m_,
                   semantics::type& t_,
                   const custom_cxx_type* ct_,
                   semantics::type* wrapper_,
                   bool cq_,
                   string& var_,
                   string const& fq_type)
          : m (m_),
            t (t_),
            ct (ct_),
            ptr (0),
            wrapper (wrapper_),
            cq (cq_),
            st (0),
            var (var_),
            fq_type_ (fq_type)
      {
      }
    };

    bool
    container (member_info& mi)
    {
      // This cannot be a container if we have a type override.
      //
      return type_override_ == 0 && context::container (mi.m);
    }

    // The false return value indicates that no further callbacks should be
    // called for this member.
    //
    virtual bool
    pre (member_info&) {return true;}

    virtual void
    post (member_info&) {}

    // Note: calling these directly will mess up the top_level logic.
    //
  protected:
    virtual void
    traverse_composite (member_info&) {}

    virtual void
    traverse_container (member_info&) {}

    // Note that by default traverse_pointer() will traverse the
    // pointed-to object id type.
    //
    virtual void
    traverse_pointer (member_info&);

    virtual void
    traverse_simple (member_info&) {}

  private:
    virtual void
    traverse (semantics::data_member&);
  };

  //
  //
  struct member_image_type: virtual member_base
  {
    typedef member_image_type base;

    member_image_type (): member_base (0, 0, string (), string ()) {}

    member_image_type (semantics::type* type,
                       const custom_cxx_type* ct,
                       string const& fq_type = string (),
                       string const& key_prefix = string ())
        : member_base (type, ct, fq_type, key_prefix) {}

    // Has to be overriden.
    //
    virtual string
    image_type (semantics::data_member&);
  };

  //
  //
  struct member_database_type_id: virtual member_base
  {
    typedef member_database_type_id base;

    member_database_type_id (): member_base (0, 0, string (), string ()) {}
    member_database_type_id (semantics::type* type,
                             const custom_cxx_type* ct,
                             string const& fq_type = string (),
                             string const& key_prefix = string ())
        : member_base (type, ct, fq_type, key_prefix) {}

    // Has to be overriden.
    //
    virtual string
    database_type_id (semantics::data_member&);
  };
}

#include <odb/relational/common.txx>

// Other common parts.
//
#include <odb/relational/common-query.hxx>

#endif // ODB_RELATIONAL_COMMON_HXX
