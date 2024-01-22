// file      : odb/relational/pgsql/common.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_PGSQL_COMMON_HXX
#define ODB_RELATIONAL_PGSQL_COMMON_HXX

#include <odb/relational/common.hxx>
#include <odb/relational/pgsql/context.hxx>

namespace relational
{
  namespace pgsql
  {
    struct member_base: virtual relational::member_base_impl<sql_type>, context
    {
      member_base (base const& x): base (x), base_impl (x) {}

      // This c-tor is for the direct use inside the pgsql namespace.
      // If you do use this c-tor, you should also explicitly call
      // relational::member_base (aka base).
      //
      member_base () {}

      virtual sql_type const&
      member_sql_type (semantics::data_member&);

      virtual void
      traverse_simple (member_info&);

      virtual void
      traverse_integer (member_info&)
      {
      }

      virtual void
      traverse_float (member_info&)
      {
      }

      virtual void
      traverse_numeric (member_info&)
      {
      }

      virtual void
      traverse_date_time (member_info&)
      {
      }

      virtual void
      traverse_string (member_info&)
      {
      }

      virtual void
      traverse_bit (member_info&)
      {
      }

      virtual void
      traverse_varbit (member_info&)
      {
      }

      virtual void
      traverse_uuid (member_info&)
      {
      }
    };

    struct member_image_type: relational::member_image_type,
                              member_base
    {
      member_image_type (base const&);
      member_image_type ();
      member_image_type (semantics::type* type,
                         const custom_cxx_type*,
                         string const& fq_type = string (),
                         string const& key_prefix = string ());
      virtual string
      image_type (semantics::data_member&);

      virtual void
      traverse_composite (member_info&);

      virtual void
      traverse_integer (member_info&);

      virtual void
      traverse_float (member_info&);

      virtual void
      traverse_numeric (member_info&);

      virtual void
      traverse_date_time (member_info&);

      virtual void
      traverse_string (member_info&);

      virtual void
      traverse_bit (member_info&);

      virtual void
      traverse_varbit (member_info&);

      virtual void
      traverse_uuid (member_info&);

    private:
      string type_;
    };

    struct member_database_type_id: relational::member_database_type_id,
                                    member_base
    {
      member_database_type_id (base const&);
      member_database_type_id ();
      member_database_type_id (semantics::type* type,
                               const custom_cxx_type*,
                               string const& fq_type = string (),
                               string const& key_prefix = string ());

      virtual string
      database_type_id (type&);

      virtual void
      traverse_composite (member_info&);

      virtual void
      traverse_integer (member_info&);

      virtual void
      traverse_float (member_info&);

      virtual void
      traverse_numeric (member_info&);

      virtual void
      traverse_date_time (member_info&);

      virtual void
      traverse_string (member_info&);

      virtual void
      traverse_bit (member_info&);

      virtual void
      traverse_varbit (member_info&);

      virtual void
      traverse_uuid (member_info&);

    private:
      string type_id_;
    };
  }
}
#endif // ODB_RELATIONAL_PGSQL_COMMON_HXX
