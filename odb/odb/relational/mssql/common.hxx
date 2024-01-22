// file      : odb/relational/mssql/common.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_MSSQL_COMMON_HXX
#define ODB_RELATIONAL_MSSQL_COMMON_HXX

#include <odb/relational/common.hxx>
#include <odb/relational/mssql/context.hxx>

namespace relational
{
  namespace mssql
  {
    struct member_base: virtual relational::member_base_impl<sql_type>, context
    {
      member_base (base const& x): base (x), base_impl (x) {}

      // This c-tor is for the direct use inside the mssql namespace.
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
      traverse_decimal (member_info&)
      {
      }

      virtual void
      traverse_smallmoney (member_info&)
      {
      }

      virtual void
      traverse_money (member_info&)
      {
      }

      virtual void
      traverse_float4 (member_info&)
      {
      }

      virtual void
      traverse_float8 (member_info&)
      {
      }

      virtual void
      traverse_string (member_info&)
      {
      }

      virtual void
      traverse_long_string (member_info&)
      {
      }

      virtual void
      traverse_nstring (member_info&)
      {
      }

      virtual void
      traverse_long_nstring (member_info&)
      {
      }

      virtual void
      traverse_binary (member_info&)
      {
      }

      virtual void
      traverse_long_binary (member_info&)
      {
      }

      virtual void
      traverse_date (member_info&)
      {
      }

      virtual void
      traverse_time (member_info&)
      {
      }

      virtual void
      traverse_datetime (member_info&)
      {
      }

      virtual void
      traverse_datetimeoffset (member_info&)
      {
      }

      virtual void
      traverse_uniqueidentifier (member_info&)
      {
      }

      virtual void
      traverse_rowversion (member_info&)
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
      traverse_decimal (member_info&);

      virtual void
      traverse_smallmoney (member_info&);

      virtual void
      traverse_money (member_info&);

      virtual void
      traverse_float4 (member_info&);

      virtual void
      traverse_float8 (member_info&);

      virtual void
      traverse_string (member_info&);

      virtual void
      traverse_long_string (member_info&);

      virtual void
      traverse_nstring (member_info&);

      virtual void
      traverse_long_nstring (member_info&);

      virtual void
      traverse_binary (member_info&);

      virtual void
      traverse_long_binary (member_info&);

      virtual void
      traverse_date (member_info&);

      virtual void
      traverse_time (member_info&);

      virtual void
      traverse_datetime (member_info&);

      virtual void
      traverse_datetimeoffset (member_info&);

      virtual void
      traverse_uniqueidentifier (member_info&);

      virtual void
      traverse_rowversion (member_info&);

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
      database_type_id (semantics::data_member&);

      virtual void
      traverse_composite (member_info&);

      virtual void
      traverse_integer (member_info&);

      virtual void
      traverse_decimal (member_info&);

      virtual void
      traverse_smallmoney (member_info&);

      virtual void
      traverse_money (member_info&);

      virtual void
      traverse_float4 (member_info&);

      virtual void
      traverse_float8 (member_info&);

      virtual void
      traverse_string (member_info&);

      virtual void
      traverse_long_string (member_info&);

      virtual void
      traverse_nstring (member_info&);

      virtual void
      traverse_long_nstring (member_info&);

      virtual void
      traverse_binary (member_info&);

      virtual void
      traverse_long_binary (member_info&);

      virtual void
      traverse_date (member_info&);

      virtual void
      traverse_time (member_info&);

      virtual void
      traverse_datetime (member_info&);

      virtual void
      traverse_datetimeoffset (member_info&);

      virtual void
      traverse_uniqueidentifier (member_info&);

      virtual void
      traverse_rowversion (member_info&);

    private:
      string type_id_;
    };

    struct has_long_data: object_columns_base, context
    {
      has_long_data (bool& r): r_ (r) {}

      virtual void
      traverse_pointer (semantics::data_member& m, semantics::class_& c)
      {
        if (!inverse (m, key_prefix_))
          object_columns_base::traverse_pointer (m, c);
      }

      virtual bool
      traverse_column (semantics::data_member& m, string const&, bool)
      {
        if (long_data (parse_sql_type (column_type (), m)))
          r_ = true;

        return true;
      }

    private:
      bool& r_;
    };
  }
}
#endif // ODB_RELATIONAL_MSSQL_COMMON_HXX
