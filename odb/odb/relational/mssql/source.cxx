// file      : odb/relational/mssql/source.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/source.hxx>

#include <odb/relational/mssql/common.hxx>
#include <odb/relational/mssql/context.hxx>

using namespace std;

namespace relational
{
  namespace mssql
  {
    namespace source
    {
      namespace relational = relational::source;

      //
      //
      struct query_parameters: relational::query_parameters
      {
        query_parameters (base const& x): base (x) {}

        virtual string
        auto_id (semantics::data_member&, const string&, const string&)
        {
          return "";
        }
      };
      entry<query_parameters> query_parameters_;

      //
      //
      struct object_columns: relational::object_columns, context
      {
        object_columns (base const& x)
          : base (x), rowversion_ (false), column_count_ (0) {}

        virtual bool
        column (semantics::data_member& m,
                string const& table,
                string const& column)
        {
          // Don't add a column for auto id in the INSERT statement.
          // Only simple, direct id can be auto.
          //
          if (sk_ == statement_insert && key_prefix_.empty () && auto_ (m))
            return false;

          // Don't update the ROWVERSION column explicitly.
          //
          if (sk_ == statement_update)
          {
            sql_type t (parse_sql_type (column_type (), m));
            if (t.type == sql_type::ROWVERSION)
            {
              rowversion_ = true;
              return false;
            }
          }

          bool r (base::column (m, table, column));

          // Count the number of columns in the UPDATE statement, but
          // excluding soft-deleted.
          //
          if (sk_ == statement_update && r && !deleted (member_path_))
            column_count_++;

          return r;
        }

        virtual void
        traverse_post (semantics::nameable& n)
        {
          if (rowversion_ && column_count_ == 0)
          {
            location l (n.location ());
            error (l) << "ROWVERSION in an object without any readwrite "
              "data members" << endl;
            error (l) << "UPDATE statement will be empty" << endl;
            throw operation_failed ();
          }
        }

      private:
        bool rowversion_;
        size_t column_count_;
      };
      entry<object_columns> object_columns_;

      //
      //
      struct persist_statement_params: relational::persist_statement_params,
                                       context
      {
        persist_statement_params (base const& x): base (x) {}

        virtual string
        version_value (semantics::data_member& m)
        {
          sql_type t (parse_sql_type (column_type (), m));
          return t.type == sql_type::ROWVERSION ? "DEFAULT" : "1";
        }
      };
      entry<persist_statement_params> persist_statement_params_;

      //
      // bind
      //

      static const char* integer_buffer_types[] =
      {
        "mssql::bind::bit",
        "mssql::bind::tinyint",
        "mssql::bind::smallint",
        "mssql::bind::int_",
        "mssql::bind::bigint"
      };

      struct bind_member: relational::bind_member_impl<sql_type>,
                          member_base
      {
        bind_member (base const& x)
            : member_base::base (x),      // virtual base
              member_base::base_impl (x), // virtual base
              base_impl (x),
              member_base (x)
        {
        }

        virtual void
        traverse_integer (member_info& mi)
        {
          os << b << ".type = " <<
            integer_buffer_types[mi.st->type - sql_type::BIT] << ";"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;";
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          os << b << ".type = mssql::bind::decimal;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode precision (p) and scale (s) as (p * 100 + s).
            //
             << b << ".capacity = " << mi.st->prec * 100 + mi.st->scale << ";";
        }

        virtual void
        traverse_smallmoney (member_info& mi)
        {
          os << b << ".type = mssql::bind::smallmoney;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;";
        }

        virtual void
        traverse_money (member_info& mi)
        {
          os << b << ".type = mssql::bind::money;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;";
        }

        virtual void
        traverse_float4 (member_info& mi)
        {
          os << b << ".type = mssql::bind::float4;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
             << b << ".capacity = " << mi.st->prec << ";";
        }

        virtual void
        traverse_float8 (member_info& mi)
        {
          os << b << ".type = mssql::bind::float8;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
             << b << ".capacity = " << mi.st->prec << ";";
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << b << ".type = mssql::bind::string;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
             << b << ".capacity = static_cast<SQLLEN> (sizeof (" <<
            arg << "." << mi.var << "value));";
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << b << ".type = mssql::bind::long_string;"
             << b << ".buffer = &" << arg << "." << mi.var << "callback;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode the column size with 0 indicating unlimited.
            //
             << b << ".capacity = " << mi.st->prec << ";";
        }

        virtual void
        traverse_nstring (member_info& mi)
        {
          os << b << ".type = mssql::bind::nstring;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
             << b << ".capacity = static_cast<SQLLEN> (sizeof (" <<
            arg << "." << mi.var << "value));";
        }

        virtual void
        traverse_long_nstring (member_info& mi)
        {
          os << b << ".type = mssql::bind::long_nstring;"
             << b << ".buffer = &" << arg << "." << mi.var << "callback;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode the column size (in bytes) with 0 indicating unlimited.
            //
             << b << ".capacity = " << mi.st->prec * 2 << ";";
        }

        virtual void
        traverse_binary (member_info& mi)
        {
          os << b << ".type = mssql::bind::binary;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
             << b << ".capacity = static_cast<SQLLEN> (sizeof (" <<
            arg << "." << mi.var << "value));";
        }

        virtual void
        traverse_long_binary (member_info& mi)
        {
          os << b << ".type = mssql::bind::long_binary;"
             << b << ".buffer = &" << arg << "." << mi.var << "callback;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode the column size with 0 indicating unlimited.
            //
             << b << ".capacity = " << mi.st->prec << ";";
        }

        virtual void
        traverse_date (member_info& mi)
        {
          os << b << ".type = mssql::bind::date;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;";
        }

        virtual void
        traverse_time (member_info& mi)
        {
          os << b << ".type = mssql::bind::time;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode fractional seconds (scale).
            //
             << b << ".capacity = " << mi.st->scale << ";";
        }

        virtual void
        traverse_datetime (member_info& mi)
        {
          unsigned short scale (0);

          switch (mi.st->type)
          {
          case sql_type::DATETIME:
            {
              // Looks like it is 3 (rounded to 0.000, 0.003, or 0.007).
              //
              scale = 3;
              break;
            }
          case sql_type::DATETIME2:
            {
              scale = mi.st->scale;
              break;
            }
          case sql_type::SMALLDATETIME:
            {
              // No seconds in SMALLDATATIME. Encode it a special precision
              // value (8).
              //
              scale = 8;
              break;
            }
          default:
            {
              assert (false);
              break;
            }
          }

          os << b << ".type = mssql::bind::datetime;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode fractional seconds (scale).
            //
             << b << ".capacity = " << scale << ";";
        }

        virtual void
        traverse_datetimeoffset (member_info& mi)
        {
          os << b << ".type = mssql::bind::datetimeoffset;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;"
            // Encode fractional seconds (scale).
            //
             << b << ".capacity = " << mi.st->scale << ";";
        }

        virtual void
        traverse_uniqueidentifier (member_info& mi)
        {
          os << b << ".type = mssql::bind::uniqueidentifier;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;";
        }

        virtual void
        traverse_rowversion (member_info& mi)
        {
          os << b << ".type = mssql::bind::rowversion;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size_ind = &" << arg << "." << mi.var << "size_ind;";
        }
      };
      entry<bind_member> bind_member_;

      //
      // init image
      //

      struct init_image_member: relational::init_image_member_impl<sql_type>,
                                member_base
      {
        init_image_member (base const& x)
            : member_base::base (x),      // virtual base
              member_base::base_impl (x), // virtual base
              base_impl (x),
              member_base (x)
        {
        }

        virtual void
        set_null (member_info& mi)
        {
          os << "i." << mi.var << "size_ind = SQL_NULL_DATA;";
        }

        virtual void
        check_accessor (member_info& mi, member_access& ma)
        {
          // We cannot use accessors that return by-value for long data
          // members.
          //
          if (long_data (*mi.st) && ma.by_value)
          {
            error (ma.loc) << "accessor returning a value cannot be used "
                           << "for a data member of SQL Server long data "
                           << "type" << endl;
            info (ma.loc) << "accessor returning a const reference is required"
                          << endl;
            info (mi.m.location ()) << "data member is defined here" << endl;
            throw operation_failed ();
          }
        }

        virtual void
        traverse_integer (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_smallmoney (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 4;";
        }

        virtual void
        traverse_money (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 8;";
        }

        virtual void
        traverse_float4 (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_float8 (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << "std::size_t size (0);"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
            // Don't mention the extra character for the null-terminator.
             << "sizeof (i." << mi.var << "value) - 1," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "size_ind =" << endl
             << "  is_null ? SQL_NULL_DATA : static_cast<SQLLEN> (size);";
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "callback.callback.param," << endl
             << "i." << mi.var << "callback.context.param," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "size_ind = is_null ? " <<
            "SQL_NULL_DATA : SQL_DATA_AT_EXEC;";
        }

        virtual void
        traverse_nstring (member_info& mi)
        {
          os << "std::size_t size (0);"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
            // Don't mention the extra character for the null-terminator.
             << "sizeof (i." << mi.var << "value) / 2 - 1," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "size_ind =" << endl
             << "  is_null ? SQL_NULL_DATA : static_cast<SQLLEN> (size * 2);";
        }

        virtual void
        traverse_long_nstring (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "callback.callback.param," << endl
             << "i." << mi.var << "callback.context.param," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "size_ind = is_null ? " <<
            "SQL_NULL_DATA : SQL_DATA_AT_EXEC;";
        }

        virtual void
        traverse_binary (member_info& mi)
        {
          os << "std::size_t size (0);"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "sizeof (i." << mi.var << "value)," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "size_ind =" << endl
             << "  is_null ? SQL_NULL_DATA : static_cast<SQLLEN> (size);";
        }

        virtual void
        traverse_long_binary (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "callback.callback.param," << endl
             << "i." << mi.var << "callback.context.param," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "size_ind = is_null ? " <<
            "SQL_NULL_DATA : SQL_DATA_AT_EXEC;";
        }

        virtual void
        traverse_date (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_time (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, " << mi.st->scale << ", " <<
            "is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null" << endl
             << "  ? SQL_NULL_DATA" << endl
             << "  : static_cast<SQLLEN> (sizeof (i." << mi.var << "value));";
        }

        virtual void
        traverse_datetime (member_info& mi)
        {
          // The same code as in bind.
          //
          unsigned short scale (0);

          switch (mi.st->type)
          {
          case sql_type::DATETIME:
            {
              scale = 3;
              break;
            }
          case sql_type::DATETIME2:
            {
              scale = mi.st->scale;
              break;
            }
          case sql_type::SMALLDATETIME:
            {
              scale = 8;
              break;
            }
          default:
            {
              assert (false);
              break;
            }
          }

          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, " << scale << ", " <<
            "is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_datetimeoffset (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, " << mi.st->scale << ", " <<
            "is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null" << endl
             << "  ? SQL_NULL_DATA" << endl
             << "  : static_cast<SQLLEN> (sizeof (i." << mi.var << "value));";
        }

        virtual void
        traverse_uniqueidentifier (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 0;";
        }

        virtual void
        traverse_rowversion (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "size_ind = is_null ? SQL_NULL_DATA : 8;";
        }
      };
      entry<init_image_member> init_image_member_;

      //
      // init value
      //

      struct init_value_member: relational::init_value_member_impl<sql_type>,
                                member_base
      {
        init_value_member (base const& x)
            : member_base::base (x),      // virtual base
              member_base::base_impl (x), // virtual base
              base_impl (x),
              member_base (x)
        {
        }

        virtual void
        get_null (string const& var) const
        {
          os << "i." << var << "size_ind == SQL_NULL_DATA";
        }

        virtual void
        check_modifier (member_info& mi, member_access& ma)
        {
          // We cannot use by-value modifier for long data members.
          //
          if (long_data (*mi.st) && ma.placeholder ())
          {
            error (ma.loc) << "modifier accepting a value cannot be used "
                           << "for a data member of SQL Server long data "
                           << "type" << endl;
            info (ma.loc) << "modifier returning a non-const reference is "
                          << "required" << endl;
            info (mi.m.location ()) << "data member is defined here" << endl;
            throw operation_failed ();
          }
        }

        virtual void
        traverse_integer (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_smallmoney (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_money (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_float4 (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_float8 (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "static_cast<std::size_t> (i." << mi.var << "size_ind)," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "callback.callback.result," << endl
             << "i." << mi.var << "callback.context.result);"
             << endl;
        }

        virtual void
        traverse_nstring (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "static_cast<std::size_t> (" <<
            "i." << mi.var << "size_ind / 2)," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_long_nstring (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "callback.callback.result," << endl
             << "i." << mi.var << "callback.context.result);"
             << endl;
        }

        virtual void
        traverse_binary (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "static_cast<std::size_t> (i." << mi.var << "size_ind)," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_long_binary (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "callback.callback.result," << endl
             << "i." << mi.var << "callback.context.result);"
             << endl;
        }

        virtual void
        traverse_date (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_time (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_datetime (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_datetimeoffset (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_uniqueidentifier (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }

        virtual void
        traverse_rowversion (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size_ind == SQL_NULL_DATA);"
             << endl;
        }
      };
      entry<init_value_member> init_value_member_;

      struct statement_columns_common: context
      {
        void
        process (relational::statement_columns& cols,
                 statement_kind sk,
                 bool dynamic)
        {
          using relational::statement_columns;

          // Long data columns must come last in the SELECT statement. If
          // this statement is going to be processed at runtime, then this
          // will be taken care of then.
          //
          if (sk != statement_select || dynamic)
            return;

          // Go over the columns list while keeping track of how many
          // columns we have examined. If the current column is long data,
          // then move it to the back. Stop once we have examined all the
          // columns.
          //
          size_t n (cols.size ());
          for (statement_columns::iterator i (cols.begin ()); n != 0; --n)
          {
            if (long_data (parse_sql_type (i->type, *i->member)))
            {
              cols.push_back (*i);
              i = cols.erase (i);
            }
            else
              ++i;
          }
        }
      };

      struct container_traits: relational::container_traits,
                               statement_columns_common
      {
        container_traits (base const& x): base (x) {}

        virtual void
        cache_result (string const&)
        {
          // Caching is not necessary since with MARS enabled SQL Server
          // can execute several interleaving statements.
          //
        }

        virtual void
        init_value_extra ()
        {
          os << "sts.select_statement ().stream_result ();"
             << endl;
        }

        virtual void
        process_statement_columns (relational::statement_columns& cols,
                                   statement_kind sk,
                                   bool dynamic)
        {
          statement_columns_common::process (cols, sk, dynamic);
        }
      };
      entry<container_traits> container_traits_;

      struct section_traits: relational::section_traits,
                             statement_columns_common
      {
        section_traits (base const& x): base (x) {}

        virtual void
        init_value_extra ()
        {
          os << "st.stream_result ();";
        }

        virtual void
        process_statement_columns (relational::statement_columns& cols,
                                   statement_kind sk,
                                   bool dynamic)
        {
          statement_columns_common::process (cols, sk, dynamic);
        }

        virtual string
        optimistic_version_increment (semantics::data_member& m)
        {
          sql_type t (parse_sql_type (column_type (m), m));
          return t.type != sql_type::ROWVERSION
            ? "1"
            : "version (sts.id_image ())";
        }

        virtual string
        update_statement_extra (user_section&)
        {
          string r;

          semantics::data_member* ver (optimistic (c_));

          if (ver == 0 ||
              parse_sql_type (column_type (*ver), *ver).type !=
              sql_type::ROWVERSION)
            return r;

          // ROWVERSION & SQL Server 2005 incompatibility is detected
          // in persist_statement_extra.
          //
          r = "OUTPUT INSERTED." +
            convert_from (column_qname (*ver, column_prefix ()), *ver);

          return r;
        }
      };
      entry<section_traits> section_traits_;

      struct class_: relational::class_, statement_columns_common
      {
        class_ (base const& x):
            base (x), init_version_value_member_id_image_ ("v", "version_") {}

        virtual void
        init_image_pre (type& c)
        {
          if (options.generate_query () &&
              !(composite (c) || (abstract (c) && !polymorphic (c))))
          {
            type* poly_root (polymorphic (c));
            bool poly_derived (poly_root != 0 && poly_root != &c);

            if (poly_derived)
              os << "{"
                 << "root_traits::image_type& ri (root_image (i));"
                 << endl;

            string i (poly_derived ? "ri" : "i");

            os << "if (" << i << ".change_callback_.callback != 0)" << endl
               << "(" << i << ".change_callback_.callback) (" <<
              i << ".change_callback_.context);";

            if (poly_derived)
              os << "}";
            else
              os << endl;
          }
        }

        virtual void
        init_value_extra ()
        {
          os << "st.stream_result ();";
        }

        virtual string
        persist_statement_extra (type& c,
                                 relational::query_parameters&,
                                 persist_position p)
        {
          string r;

          type* poly_root (polymorphic (c));
          bool poly_derived (poly_root != 0 && poly_root != &c);

          // If we are a derived type in a polymorphic hierarchy, then
          // auto id/version are handled by the root.
          //
          if (poly_derived)
            return r;

          // See if we have auto id or ROWVERSION version.
          //
          data_member_path* id (id_member (c));
          semantics::data_member* ver (optimistic (c));

          if (id != 0 && !auto_ (*id))
            id = 0;

          if (ver != 0)
          {
            sql_type t (parse_sql_type (column_type (*ver), *ver));
            if (t.type != sql_type::ROWVERSION)
              ver = 0;
          }

          if (id == 0 && ver == 0)
            return r;

          // SQL Server 2005 has a bug that causes it to fail on an
          // INSERT statement with the OUTPUT clause if data for one
          // of the inserted columns is supplied at execution (long
          // data). To work around this problem we use the less
          // efficient batch of INSERT and SELECT statements.
          //
          if (options.mssql_server_version () <= mssql_version (9, 0))
          {
            bool ld (false);

            if (c.count ("mssql-has-long-data"))
              ld = c.get<bool> ("mssql-has-long-data");
            else
            {
              has_long_data t (ld);
              t.traverse (c);
              c.set ("mssql-has-long-data", ld);
            }

            if (ld)
            {
              if (p == persist_after_values)
              {
                // SQL Server 2005 has no eqivalent of SCOPE_IDENTITY for
                // ROWVERSION.
                //
                if (ver != 0)
                {
                  error (c.location ()) << "in SQL Server 2005 ROWVERSION " <<
                    "value cannot be retrieved for a persistent class " <<
                    "containing long data" << endl;
                  throw operation_failed ();
                }

                // We also cannot support bulk INSERT.
                //
                if (c.count ("bulk-persist"))
                {
                  error (c.location ()) << "in SQL Server 2005 bulk " <<
                    "persist operation cannot be implemented for a " <<
                    "persistent class containing long data" << endl;
                  throw operation_failed ();
                }

                r = "; SELECT " +
                  convert_from ("SCOPE_IDENTITY()", *id->back ());
              }

              return r;
            }
          }

          if (p == persist_after_columns)
          {
            r = "OUTPUT ";

            // Top-level auto id column.
            //
            if (id != 0)
              r += "INSERTED." +
                convert_from (column_qname (*id), *id->back ());

            // Top-level version column.
            //
            if (ver != 0)
            {
              if (id != 0)
                r += ',';

              r += "INSERTED." + convert_from (
                column_qname (*ver, column_prefix ()), *ver);
            }
          }

          return r;
        }

        virtual string
        update_statement_extra (type& c)
        {
          string r;

          type* poly_root (polymorphic (c));
          bool poly_derived (poly_root != 0 && poly_root != &c);

          // If we are a derived type in a polymorphic hierarchy, then
          // version is handled by the root.
          //
          if (poly_derived)
            return r;

          semantics::data_member* ver (optimistic (c));

          if (ver == 0 ||
              parse_sql_type (column_type (*ver), *ver).type !=
              sql_type::ROWVERSION)
            return r;

          // Long data & SQL Server 2005 incompatibility is detected
          // in persist_statement_extra.
          //
          r = "OUTPUT INSERTED." +
            convert_from (column_qname (*ver, column_prefix ()), *ver);

          return r;
        }

        virtual void
        process_statement_columns (relational::statement_columns& cols,
                                   statement_kind sk,
                                   bool dynamic)
        {
          statement_columns_common::process (cols, sk, dynamic);
        }

        virtual string
        optimistic_version_init (semantics::data_member& m, bool index)
        {
          sql_type t (parse_sql_type (column_type (m), m));
          return t.type != sql_type::ROWVERSION
            ? "1"
            : (index
               ? "version (sts.id_image (i))"
               : "version (sts.id_image ())");
        }

        virtual string
        optimistic_version_increment (semantics::data_member& m, bool index)
        {
          sql_type t (parse_sql_type (column_type (m), m));
          return t.type != sql_type::ROWVERSION
            ? "1"
            : (index
               ? "version (sts.id_image (i))"
               : "version (sts.id_image ())");
        }

        virtual bool
        optimistic_insert_bind_version (semantics::data_member& m)
        {
          sql_type t (parse_sql_type (column_type (m), m));
          return t.type == sql_type::ROWVERSION;
        }

        virtual void
        object_extra (type& c)
        {
          bool abst (abstract (c));

          type* poly_root (polymorphic (c));
          bool poly (poly_root != 0);
          bool poly_derived (poly && poly_root != &c);

          if (poly_derived || (abst && !poly))
            return;

          if (semantics::data_member* m = optimistic (c))
          {
            sql_type t (parse_sql_type (column_type (*m), *m));
            if (t.type == sql_type::ROWVERSION)
            {
              string const& type (class_fq_name (c));
              string traits ("access::object_traits_impl< " + type + ", id_" +
                             db.string () + " >");

              os << traits << "::version_type" << endl
                 << traits << "::" << endl
                 << "version (const id_image_type& i)"
                 << "{"
                 << "version_type v;";
              init_version_value_member_id_image_->traverse (*m);
              os << "return v;"
                 << "}";
            }
          }
        }

        virtual string
        from_trailer (type& c)
        {
          return c.get<view_query> ("query").for_update
            ? " WITH (UPDLOCK)"
            : "";
        }

        virtual string
        select_trailer (type&) {return "";}

      private:
        // Go via the dynamic creation to get access to the constructor.
        //
        instance<relational::init_value_member>
          init_version_value_member_id_image_;
      };
      entry<class_> class_entry_;
    }
  }
}
