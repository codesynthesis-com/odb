// file      : odb/relational/mssql/common.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>

#include <odb/relational/mssql/common.hxx>

using namespace std;

namespace relational
{
  namespace mssql
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
      const sql_type& st (*mi.st);

      // The same long/short data test as in context.cxx:long_data().
      //
      switch (st.type)
      {
        // Integral types.
        //
      case sql_type::BIT:
      case sql_type::TINYINT:
      case sql_type::SMALLINT:
      case sql_type::INT:
      case sql_type::BIGINT:
        {
          traverse_integer (mi);
          break;
        }

        // Fixed and floating point types.
        //
      case sql_type::DECIMAL:
        {
          traverse_decimal (mi);
          break;
        }
      case sql_type::SMALLMONEY:
        {
          traverse_smallmoney (mi);
          break;
        }
      case sql_type::MONEY:
        {
          traverse_money (mi);
          break;
        }
      case sql_type::FLOAT:
        {
          if (st.prec > 24)
            traverse_float8 (mi);
          else
            traverse_float4 (mi);

          break;
        }

        // String and binary types.
        //
      case sql_type::CHAR:
      case sql_type::VARCHAR:
        {
          // Zero precision means max in VARCHAR(max).
          //
          if (st.prec == 0 || st.prec > options.mssql_short_limit ())
            traverse_long_string (mi);
          else
            traverse_string (mi);

          break;
        }
      case sql_type::TEXT:
        {
          traverse_long_string (mi);
          break;
        }
      case sql_type::NCHAR:
      case sql_type::NVARCHAR:
        {
          // Zero precision means max in NVARCHAR(max). Note that
          // the precision is in 2-byte UCS-2 characters, not bytes.
          //
          if (st.prec == 0 || st.prec * 2 > options.mssql_short_limit ())
            traverse_long_nstring (mi);
          else
            traverse_nstring (mi);

          break;
        }
      case sql_type::NTEXT:
        {
          traverse_long_nstring (mi);
          break;
        }
      case sql_type::BINARY:
      case sql_type::VARBINARY:
        {
          // Zero precision means max in VARCHAR(max).
          //
          if (st.prec == 0 || st.prec > options.mssql_short_limit ())
            traverse_long_binary (mi);
          else
            traverse_binary (mi);

          break;
        }
      case sql_type::IMAGE:
        {
          traverse_long_binary (mi);
          break;
        }

        // Date-time types.
        //
      case sql_type::DATE:
        {
          traverse_date (mi);
          break;
        }
      case sql_type::TIME:
        {
          traverse_time (mi);
          break;
        }
      case sql_type::DATETIME:
      case sql_type::DATETIME2:
      case sql_type::SMALLDATETIME:
        {
          traverse_datetime (mi);
          break;
        }
      case sql_type::DATETIMEOFFSET:
        {
          traverse_datetimeoffset (mi);
          break;
        }

        // Other types.
        //
      case sql_type::UNIQUEIDENTIFIER:
        {
          traverse_uniqueidentifier (mi);
          break;
        }
      case sql_type::ROWVERSION:
        {
          traverse_rowversion (mi);
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

    static const char* integer_types[] =
    {
      "unsigned char",
      "unsigned char",
      "short",
      "int",
      "long long"
    };

    member_image_type::
    member_image_type (base const& x)
        : member_base::base (x), // virtual base
          base (x) {}

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
        ", id_mssql >::image_type";
    }

    void member_image_type::
    traverse_integer (member_info& mi)
    {
      type_ = integer_types[mi.st->type - sql_type::BIT];
    }

    void member_image_type::
    traverse_decimal (member_info&)
    {
      type_ = "mssql::decimal";
    }

    void member_image_type::
    traverse_smallmoney (member_info&)
    {
      type_ = "mssql::smallmoney";
    }

    void member_image_type::
    traverse_money (member_info&)
    {
      type_ = "mssql::money";
    }

    void member_image_type::
    traverse_float4 (member_info&)
    {
      type_ = "float";
    }

    void member_image_type::
    traverse_float8 (member_info&)
    {
      type_ = "double";
    }

    void member_image_type::
    traverse_string (member_info&)
    {
      type_ = "char*";
    }

    void member_image_type::
    traverse_long_string (member_info&)
    {
      type_ = "mssql::long_callback";
    }

    void member_image_type::
    traverse_nstring (member_info&)
    {
      type_ = "mssql::ucs2_char*";
    }

    void member_image_type::
    traverse_long_nstring (member_info&)
    {
      type_ = "mssql::long_callback";
    }

    void member_image_type::
    traverse_binary (member_info&)
    {
      type_ = "char*";
    }

    void member_image_type::
    traverse_long_binary (member_info&)
    {
      type_ = "mssql::long_callback";
    }

    void member_image_type::
    traverse_date (member_info&)
    {
      type_ = "mssql::date";
    }

    void member_image_type::
    traverse_time (member_info&)
    {
      type_ = "mssql::time";
    }

    void member_image_type::
    traverse_datetime (member_info&)
    {
      type_ = "mssql::datetime";
    }

    void member_image_type::
    traverse_datetimeoffset (member_info&)
    {
      type_ = "mssql::datetimeoffset";
    }

    void member_image_type::
    traverse_uniqueidentifier (member_info&)
    {
      type_ = "mssql::uniqueidentifier";
    }

    void member_image_type::
    traverse_rowversion (member_info&)
    {
      type_ = "unsigned char*";
    }

    entry<member_image_type> member_image_type_;

    //
    // member_database_type
    //

    static const char* integer_database_id[] =
    {
      "mssql::id_bit",
      "mssql::id_tinyint",
      "mssql::id_smallint",
      "mssql::id_int",
      "mssql::id_bigint"
    };

    member_database_type_id::
    member_database_type_id (base const& x)
        : member_base::base (x), // virtual base
          base (x)
    {
    }

    member_database_type_id::
    member_database_type_id ()
        : member_base::base (0, 0, string (), string ()), // virtual base
          base (0, 0, string (), string ())
    {
    }

    member_database_type_id::
    member_database_type_id (semantics::type* type,
                             const custom_cxx_type* ct,
                             string const& fq_type,
                             string const& key_prefix)
        : member_base::base (type, ct, fq_type, key_prefix), // virtual base
          base (type, ct, fq_type, key_prefix)
    {
    }

    string member_database_type_id::
    database_type_id (semantics::data_member& m)
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
      type_id_ = integer_database_id[mi.st->type - sql_type::BIT];
    }

    void member_database_type_id::
    traverse_decimal (member_info&)
    {
      type_id_ = "mssql::id_decimal";
    }

    void member_database_type_id::
    traverse_smallmoney (member_info&)
    {
      type_id_ = "mssql::id_smallmoney";
    }

    void member_database_type_id::
    traverse_money (member_info&)
    {
      type_id_ = "mssql::id_money";
    }

    void member_database_type_id::
    traverse_float4 (member_info&)
    {
      type_id_ = "mssql::id_float4";
    }

    void member_database_type_id::
    traverse_float8 (member_info&)
    {
      type_id_ = "mssql::id_float8";
    }

    void member_database_type_id::
    traverse_string (member_info&)
    {
      type_id_ = "mssql::id_string";
    }

    void member_database_type_id::
    traverse_long_string (member_info&)
    {
      type_id_ = "mssql::id_long_string";
    }

    void member_database_type_id::
    traverse_nstring (member_info&)
    {
      type_id_ = "mssql::id_nstring";
    }

    void member_database_type_id::
    traverse_long_nstring (member_info&)
    {
      type_id_ = "mssql::id_long_nstring";
    }

    void member_database_type_id::
    traverse_binary (member_info&)
    {
      type_id_ = "mssql::id_binary";
    }

    void member_database_type_id::
    traverse_long_binary (member_info&)
    {
      type_id_ = "mssql::id_long_binary";
    }

    void member_database_type_id::
    traverse_date (member_info&)
    {
      type_id_ = "mssql::id_date";
    }

    void member_database_type_id::
    traverse_time (member_info&)
    {
      type_id_ = "mssql::id_time";
    }

    void member_database_type_id::
    traverse_datetime (member_info&)
    {
      type_id_ = "mssql::id_datetime";
    }

    void member_database_type_id::
    traverse_datetimeoffset (member_info&)
    {
      type_id_ = "mssql::id_datetimeoffset";
    }

    void member_database_type_id::
    traverse_uniqueidentifier (member_info&)
    {
      type_id_ = "mssql::id_uniqueidentifier";
    }

    void member_database_type_id::
    traverse_rowversion (member_info&)
    {
      type_id_ = "mssql::id_rowversion";
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

      virtual void
      column_ctor (string const& type, string const& name, string const& base)
      {
        os << name << " (";

        if (multi_dynamic)
          os << "odb::query_column< " << type << " >& qc," << endl;

        os << "const char* t," << endl
           << "const char* c," << endl
           << "const char* conv," << endl
           << "unsigned short p = 0," << endl
           << "unsigned short s = 0xFFFF)" << endl
           << "  : " << base << " (" << (multi_dynamic ? "qc, " : "") <<
          "t, c, conv, p, s)"
           << "{"
           << "}";
      }

      virtual void
      column_ctor_args_extra (semantics::data_member& m)
      {
        // For some types we need to pass precision and scale.
        //
        sql_type const& st (parse_sql_type (column_type (), m));

        switch (st.type)
        {
        case sql_type::DECIMAL:
          {
            os << ", " << st.prec << ", " << st.scale;
            break;
          }
        case sql_type::FLOAT:
          {
            os << ", " << st.prec;
            break;
          }
        case sql_type::CHAR:
        case sql_type::VARCHAR:
          {
            os << ", " << st.prec;
            break;
          }
        case sql_type::TEXT:
          {
            os << ", 0"; // Unlimited.
            break;
          }
        case sql_type::NCHAR:
        case sql_type::NVARCHAR:
          {
            os << ", " << st.prec; // In 2-byte characters.
            break;
          }
        case sql_type::NTEXT:
          {
            os << ", 0"; // Unlimited.
            break;
          }
        case sql_type::BINARY:
        case sql_type::VARBINARY:
          {
            os << ", " << st.prec;
            break;
          }
        case sql_type::IMAGE:
          {
            os << ", 0"; // Unlimited.
            break;
          }
          // Date-time types.
          //
        case sql_type::TIME:
        case sql_type::DATETIME2:
        case sql_type::DATETIMEOFFSET:
          {
            os << ", 0, " << st.scale; // Fractional seconds (scale).
            break;
          }
        case sql_type::DATETIME:
          {
            os << ", 0, 3";
            break;
          }
        case sql_type::SMALLDATETIME:
          {
            os << ", 0, 8";
            break;
          }
        default:
          {
            break;
          }
        }
      }

    private:
      member_database_type_id member_database_type_id_;
    };
    entry<query_columns> query_columns_;
  }
}
