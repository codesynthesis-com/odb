// file      : odb/relational/oracle/common.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <cassert>

#include <odb/relational/oracle/common.hxx>

using namespace std;

namespace relational
{
  namespace oracle
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
        // Numeric types.
        //
      case sql_type::NUMBER:
        {
          const sql_type& st (*mi.st);

          if (st.prec)
          {
            unsigned short r (st.prec_value);

            if (!st.scale)
            {
              if (r <= 10)
                traverse_int32 (mi);
              // Only OCI versions 11.2 and later support insertion and
              // extraction into a 64 bit integer.
              //
              else if (
                (options.oracle_client_version () >= oracle_version (11, 2)) &&
                (r <= 19 || (r == 20 && unsigned_integer (mi.t))))
                traverse_int64 (mi);
              else
                traverse_big_int (mi);
            }
            else
            {
              // We can calculate the decimal exponent of the normalised
              // floating point equivalent of the fixed point number using
              // e = p - s, where p is the precision, s is the scale, and
              // e the exponent. We can then use this to determine whether
              // or not a value of Oracle SQL type NUMBER can be completely
              // stored in the native floating point type.
              //

              // The maximum decimal precision of a float is 7 significant
              // digits. The minimum and maximum decimal exponents
              // representable by a float are -37 and 38 respectively.
              //
              if (r <= 7)
              {
                int e = r - st.scale_value;

                if (e >= -37 && e <= 38)
                  traverse_float (mi);
                else
                  traverse_double (mi);
              }

              // The maximum decimal precision of a double is 15 significant
              // digits. The minimum and maximum decimal exponent representable
              // by a double exceeds that of the Oracle NUMBER type.
              //
              else if (r <= 15)
                traverse_double (mi);
              else
                traverse_big_float (mi);
            }
          }
          else
            // If there is no precision, then this is a floating-point number.
            //
            traverse_double (mi);

          break;
        }
      case sql_type::FLOAT:
        {
          // We map FLOAT types based exclusively on their binary precision
          // seeing that in 99% of cases it is the precision that is the
          // limiting factor and not the exponent.
          //
          if (mi.st->prec_value <= 24)
            traverse_float (mi);
          else if (mi.st->prec_value <= 53)
            traverse_double (mi);
          else
            traverse_big_float (mi);

          break;
        }
      case sql_type::BINARY_FLOAT:
        {
          traverse_float (mi);
          break;
        }
      case sql_type::BINARY_DOUBLE:
        {
          traverse_double (mi);
          break;
        }
        // Data-time types.
        //
      case sql_type::DATE:
        {
          traverse_date (mi);
          break;
        }
      case sql_type::TIMESTAMP:
        {
          traverse_timestamp (mi);
          break;
        }
      case sql_type::INTERVAL_YM:
        {
          traverse_interval_ym (mi);
          break;
        }
      case sql_type::INTERVAL_DS:
        {
          traverse_interval_ds (mi);
          break;
        }
        // String and binary types.
        //
      case sql_type::CHAR:
      case sql_type::NCHAR:
      case sql_type::VARCHAR2:
      case sql_type::NVARCHAR2:
      case sql_type::RAW:
        {
          traverse_string (mi);
          break;
        }
      case sql_type::BLOB:
      case sql_type::CLOB:
      case sql_type::NCLOB:
        {
          traverse_lob (mi);
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
        ", id_oracle >::image_type";
    }

    void member_image_type::
    traverse_int32 (member_info& mi)
    {
      if (unsigned_integer (mi.t))
        type_ = "unsigned int";
      else
        type_ = "int";
    }

    void member_image_type::
    traverse_int64 (member_info& mi)
    {
      if (unsigned_integer (mi.t))
        type_ = "unsigned long long";
      else
        type_ = "long long";
    }

    void member_image_type::
    traverse_big_int (member_info&)
    {
      type_ = "char*";
    }

    void member_image_type::
    traverse_float (member_info&)
    {
      type_ = "float";
    }

    void member_image_type::
    traverse_double (member_info&)
    {
      type_ = "double";
    }

    void member_image_type::
    traverse_big_float (member_info&)
    {
      type_ = "char*";
    }

    void member_image_type::
    traverse_date (member_info&)
    {
      type_ = "char*";
    }

    void member_image_type::
    traverse_timestamp (member_info&)
    {
      type_ = "oracle::datetime";
    }

    void member_image_type::
    traverse_interval_ym (member_info&)
    {
      type_ = "oracle::interval_ym";
    }

    void member_image_type::
    traverse_interval_ds (member_info&)
    {
      type_ = "oracle::interval_ds";
    }

    void member_image_type::
    traverse_string (member_info&)
    {
      type_ = "char*";
    }

    void member_image_type::
    traverse_lob (member_info&)
    {
      type_ = "oracle::lob_callback";
    }

    entry<member_image_type> member_image_type_;

    //
    // member_database_type
    //

    namespace
    {
      const char* string_bin_database_id[] =
      {
        "id_string",   // CHAR
        "id_nstring",  // NCHAR
        "id_string",   // VARCHAR2
        "id_nstring",  // NVARCHAR2
        "id_raw"       // RAW
      };

      const char* lob_database_id[] =
      {
        "id_blob",
        "id_clob",
        "id_nclob"
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
    traverse_int32 (member_info&)
    {
      type_id_ = "oracle::id_int32";
    }

    void member_database_type_id::
    traverse_int64 (member_info&)
    {
      type_id_ = "oracle::id_int64";
    }

    void member_database_type_id::
    traverse_big_int (member_info&)
    {
      type_id_ = "oracle::id_big_int";
    }

    void member_database_type_id::
    traverse_float (member_info&)
    {
      type_id_ = "oracle::id_float";
    }

    void member_database_type_id::
    traverse_double (member_info&)
    {
      type_id_ = "oracle::id_double";
    }

    void member_database_type_id::
    traverse_big_float (member_info&)
    {
      type_id_ = "oracle::id_big_float";
    }

    void member_database_type_id::
    traverse_date (member_info&)
    {
      type_id_ = "oracle::id_date";
    }

    void member_database_type_id::
    traverse_timestamp (member_info&)
    {
      type_id_ = "oracle::id_timestamp";
    }

    void member_database_type_id::
    traverse_interval_ym (member_info&)
    {
      type_id_ = "oracle::id_interval_ym";
    }

    void member_database_type_id::
    traverse_interval_ds (member_info&)
    {
      type_id_ = "oracle::id_interval_ds";
    }

    void member_database_type_id::
    traverse_string (member_info& mi)
    {
      type_id_ = string ("oracle::") +
        string_bin_database_id[mi.st->type - sql_type::CHAR];
    }

    void member_database_type_id::
    traverse_lob (member_info& mi)
    {
      type_id_ = string ("oracle::") +
        lob_database_id[mi.st->type - sql_type::BLOB];
    }

    entry<member_database_type_id> member_database_type_id_;

    //
    // query_columns
    //

    struct query_columns: relational::query_columns, context
    {
      query_columns (base const& x): base_impl (x) {}

      void
      column_ctor (string const& type, string const& name, string const& base)
      {
        os << name << " (";

        if (multi_dynamic)
          os << "odb::query_column< " << type << " >& qc," << endl;

        os << "const char* t," << endl
           << "const char* c," << endl
           << "const char* conv," << endl
           << "unsigned short p = 0xFFF," << endl
           << "short s = 0xFFF)" << endl
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
        case sql_type::NUMBER:
          {
            if (st.prec)
            {
              os << ", " << st.prec_value;

              if (st.scale)
                os << ", " << st.scale_value;
            }
            break;
          }
        case sql_type::FLOAT:
          {
            os << ", " << st.prec_value;
            break;
          }
        case sql_type::TIMESTAMP:
          {
            os << ", " << st.prec_value;
            break;
          }
        case sql_type::INTERVAL_YM:
          {
            os << ", " << st.prec_value;
            break;
          }
        case sql_type::INTERVAL_DS:
          {
            // INTERVAL DAY TO SECOND has two precisions.
            //
            os << ", " << st.prec_value << ", " << st.scale_value;
            break;
          }
        case sql_type::CHAR:
        case sql_type::NCHAR:
        case sql_type::VARCHAR2:
        case sql_type::NVARCHAR2:
        case sql_type::RAW:
          {
            // The same logic as in header.cxx.
            //
            size_t n (st.prec ? st.prec_value : 1);

            if (!st.byte_semantics)
              n *= 4;

            if (st.type == sql_type::VARCHAR2 ||
                st.type == sql_type::NVARCHAR2)
              n = n > 4000 ? 4000 : n;
            else
              n = n > 2000 ? 2000 : n;

            os << ", " << n;
            break;
          }
        default:
          {
            break;
          }
        }
      }

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
