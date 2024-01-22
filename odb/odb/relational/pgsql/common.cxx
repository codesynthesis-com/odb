// file      : odb/relational/pgsql/common.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>

#include <odb/relational/pgsql/common.hxx>

using namespace std;

namespace relational
{
  namespace pgsql
  {
    //
    // member_base
    //

    sql_type const& member_base::
    member_sql_type (semantics::data_member& m)
    {
      return parse_sql_type (column_type (m, key_prefix_), m);
    }

    void member_base::
    traverse_simple (member_info& mi)
    {
      switch (mi.st->type)
      {
        // Integral types.
        //
      case sql_type::BOOLEAN:
      case sql_type::SMALLINT:
      case sql_type::INTEGER:
      case sql_type::BIGINT:
        {
          traverse_integer (mi);
          break;
        }

        // Float types.
        //
      case sql_type::REAL:
      case sql_type::DOUBLE:
        {
          traverse_float (mi);
          break;
        }
      case sql_type::NUMERIC:
        {
          traverse_numeric (mi);
          break;
        }

        // Data-time types.
        //
      case sql_type::DATE:
      case sql_type::TIME:
      case sql_type::TIMESTAMP:
        {
          traverse_date_time (mi);
          break;
        }

        // String and binary types.
        //
      case sql_type::CHAR:
      case sql_type::VARCHAR:
      case sql_type::TEXT:
      case sql_type::BYTEA:
        {
          traverse_string (mi);
          break;
        }
      case sql_type::BIT:
        {
          traverse_bit (mi);
          break;
        }
      case sql_type::VARBIT:
        {
          traverse_varbit (mi);
          break;
        }
        // Other types.
        //
      case sql_type::UUID:
        {
          traverse_uuid (mi);
          break;
        }
      case sql_type::invalid:
        {
          assert (false);
          break;
        }
      }
    }

    //
    // member_image_type
    //

    namespace
    {
      const char* integer_types[] =
      {
        "bool",
        "short",
        "int",
        "long long"
      };

      const char* float_types[] =
      {
        "float",
        "double"
      };

      const char* date_time_types[] =
      {
        "int",
        "long long",
        "long long"
      };
    }

    member_image_type::
    member_image_type (base const& x)
        : member_base::base (x), // virtual base
          base (x)
    {
    }

    member_image_type::
    member_image_type ()
        : relational::member_base (0, 0, string (), string ()) {}

    member_image_type::
    member_image_type (semantics::type* type,
                       const custom_cxx_type* ct,
                       string const& fq_type,
                       string const& key_prefix)
        : relational::member_base (type, ct, fq_type, key_prefix) {}

    string member_image_type::
    image_type (semantics::data_member& m)
    {
      type_.clear ();
      member_base::traverse (m, true);
      return type_;
    }

    void member_image_type::
    traverse_composite (member_info& mi)
    {
      type_ = "composite_value_traits< " + mi.fq_type () +
        ", id_pgsql >::image_type";
    }

    void member_image_type::
    traverse_integer (member_info& mi)
    {
      type_ += integer_types[mi.st->type - sql_type::BOOLEAN];
    }

    void member_image_type::
    traverse_float (member_info& mi)
    {
      type_ = float_types[mi.st->type - sql_type::REAL];
    }

    void member_image_type::
    traverse_numeric (member_info&)
    {
      type_ = "details::buffer";
    }

    void member_image_type::
    traverse_date_time (member_info& mi)
    {
      type_ = date_time_types[mi.st->type - sql_type::DATE];
    }

    void member_image_type::
    traverse_string (member_info&)
    {
      type_ = "details::buffer";
    }

    void member_image_type::
    traverse_bit (member_info&)
    {
      type_ = "unsigned char*";
    }

    void member_image_type::
    traverse_varbit (member_info&)
    {
      type_ = "details::ubuffer";
    }

    void member_image_type::
    traverse_uuid (member_info&)
    {
      type_ = "unsigned char*";
    }

    entry<member_image_type> member_image_type_;

    //
    // member_database_type
    //

    namespace
    {
      const char* integer_database_id[] =
      {
        "id_boolean",
        "id_smallint",
        "id_integer",
        "id_bigint"
      };

      const char* float_database_id[] =
      {
        "id_real",
        "id_double"
      };

      const char* date_time_database_id[] =
      {
        "id_date",
        "id_time",
        "id_timestamp"
      };

      const char* char_bin_database_id[] =
      {
        "id_string",  // CHAR
        "id_string",  // VARCHAR
        "id_string",  // TEXT,
        "id_bytea"    // BYTEA
      };
    }

    member_database_type_id::
    member_database_type_id (base const& x)
        : member_base::base (x), // virtual base
          base (x) {}

    member_database_type_id::
    member_database_type_id ()
        : member_base::base (0, 0, string (), string ()), // virtual base
          base (0, 0, string (), string ()) {}

    member_database_type_id::
    member_database_type_id (semantics::type* type,
                             const custom_cxx_type* ct,
                             string const& fq_type,
                             string const& key_prefix)
        : member_base::base (type, ct, fq_type, key_prefix), // virtual base
          base (type, ct, fq_type, key_prefix) {}

    string member_database_type_id::
    database_type_id (type& m)
    {
      type_id_.clear ();
      member_base::traverse (m, true);
      return type_id_;
    }

    void member_database_type_id::
    traverse_composite (member_info&)
    {
      assert (false);
    }

    void member_database_type_id::
    traverse_integer (member_info& mi)
    {
      type_id_ = string ("pgsql::") +
        integer_database_id[mi.st->type - sql_type::BOOLEAN];
    }

    void member_database_type_id::
    traverse_float (member_info& mi)
    {
      type_id_ = string ("pgsql::") +
        float_database_id[mi.st->type - sql_type::REAL];
    }

    void member_database_type_id::
    traverse_numeric (member_info&)
    {
      type_id_ = "pgsql::id_numeric";
    }

    void member_database_type_id::
    traverse_date_time (member_info& mi)
    {
      type_id_ = string ("pgsql::") +
        date_time_database_id[mi.st->type - sql_type::DATE];
    }

    void member_database_type_id::
    traverse_string (member_info& mi)
    {
      type_id_ = string ("pgsql::") +
        char_bin_database_id[mi.st->type - sql_type::CHAR];
    }

    void member_database_type_id::
    traverse_bit (member_info&)
    {
      type_id_ = "pgsql::id_bit";
    }

    void member_database_type_id::
    traverse_varbit (member_info&)
    {
      type_id_ = "pgsql::id_varbit";
    }

    void member_database_type_id::
    traverse_uuid (member_info&)
    {
      type_id_ = "pgsql::id_uuid";
    }

    entry<member_database_type_id> member_database_type_id_;

    //
    // query_columns
    //

    struct query_columns: relational::query_columns, context
    {
      query_columns (base const& x): base_impl (x) {}

      virtual string
      database_type_id (semantics::data_member& m)
      {
        return member_database_type_id_.database_type_id (m);
      }

    private:
      member_database_type_id member_database_type_id_;
    };
    entry<query_columns> query_columns_;
  }
}
