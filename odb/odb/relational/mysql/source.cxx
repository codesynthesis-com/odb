// file      : odb/relational/mysql/source.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/source.hxx>

#include <odb/relational/mysql/common.hxx>
#include <odb/relational/mysql/context.hxx>

using namespace std;

namespace relational
{
  namespace mysql
  {
    namespace source
    {
      namespace relational = relational::source;

      namespace
      {
        const char* integer_buffer_types[] =
        {
          "MYSQL_TYPE_TINY",
          "MYSQL_TYPE_SHORT",
          "MYSQL_TYPE_LONG",     // *_bind_param() doesn't support INT24.
          "MYSQL_TYPE_LONG",
          "MYSQL_TYPE_LONGLONG"
        };

        const char* float_buffer_types[] =
        {
          "MYSQL_TYPE_FLOAT",
          "MYSQL_TYPE_DOUBLE"
        };

        const char* date_time_buffer_types[] =
        {
          "MYSQL_TYPE_DATE",
          "MYSQL_TYPE_TIME",
          "MYSQL_TYPE_DATETIME",
          "MYSQL_TYPE_TIMESTAMP",
          "MYSQL_TYPE_SHORT"
        };

        const char* char_bin_buffer_types[] =
        {
          "MYSQL_TYPE_STRING", // CHAR
          "MYSQL_TYPE_BLOB",   // BINARY,
          "MYSQL_TYPE_STRING", // VARCHAR
          "MYSQL_TYPE_BLOB",   // VARBINARY
          "MYSQL_TYPE_STRING", // TINYTEXT
          "MYSQL_TYPE_BLOB",   // TINYBLOB
          "MYSQL_TYPE_STRING", // TEXT
          "MYSQL_TYPE_BLOB",   // BLOB
          "MYSQL_TYPE_STRING", // MEDIUMTEXT
          "MYSQL_TYPE_BLOB",   // MEDIUMBLOB
          "MYSQL_TYPE_STRING", // LONGTEXT
          "MYSQL_TYPE_BLOB"    // LONGBLOB
        };
      }

      //
      //
      struct object_columns: relational::object_columns, context
      {
        object_columns (base const& x): base (x) {}

        virtual bool
        column (semantics::data_member& m,
                string const& table,
                string const& column)
        {
          // When we store a ENUM column in the MySQL database, if we bind
          // an integer parameter, then it is treated as an index and if we
          // bind a string, then it is treated as a enumerator. Everything
          // would have worked well if the same logic applied to the select
          // operation. That is, if we bind integer, then the database sends
          // the index and if we bind string then the database sends the
          // enumerator. Unfortunately, MySQL always sends the enumerator
          // and to get the index one has to resort to the enum+0 hack.
          //
          // This causes the following problem: at code generation time we
          // do not yet know which format we want. This is determined at
          // C++ compile time by traits (the reason we don't know this is
          // because we don't want to drag database-specific runtimes,
          // which define the necessary traits, as well as their
          // prerequisites into the ODB compilation process). As a result,
          // we cannot decide at code generation time whether we need the
          // +0 hack or not. One way to overcome this would be to construct
          // the SELECT statements at runtime, something along these lines:
          //
          // "enum" + enum_traits<type>::hack + ","
          //
          // However, this complicates the code generator quite a bit: we
          // either have to move to std::string storage for all the
          // statements and all the databases, which is kind of a waste,
          // or do some deep per-database customizations, which is hairy.
          // So, instead, we are going to use another hack (hey, what the
          // hell, right?) by loading both the index and enumerator
          // combined into a string:
          //
          // CONCAT (enum+0, ' ', enum)
          //
          // For cases where we need the index, everything works since
          // MySQL will convert the leading number and stop at the space.
          // For cases where we need the enumerator, we do a bit of pre-
          // processing (see enum_traits) before handing the value off
          // to value_traits.
          //

          string const& type (column_type ());

          if (sk_ != statement_select ||
              parse_sql_type (type, m).type != sql_type::ENUM)
          {
            return base::column (m, table, column);
          }

          // Qualified column and conversion expression.
          //
          string qc;
          if (!table.empty ())
          {
            qc += table;
            qc += '.';
          }
          qc += column;
          qc = convert_from (qc, type, m);

          string r ("CONCAT(" + qc + "+0,' '," + qc + ")");

          sc_.push_back (
            relational::statement_column (table, r, type, m, key_prefix_));
          return true;
        }
      };
      entry<object_columns> object_columns_;

      struct view_columns: relational::view_columns, context
      {
        view_columns (base const& x): base (x) {}

        virtual bool
        column (semantics::data_member& m,
                string const& table,
                string const& column)
        {
          // The same idea as in object_columns.
          //
          string const& type (column_type ());

          if (parse_sql_type (type, m).type != sql_type::ENUM)
          {
            return base::column (m, table, column);
          }

          // Column and conversion expression.
          //
          string c (convert_from (column, type, m));

          string r ("CONCAT(" + c + "+0,' '," + c + ")");
          sc_.push_back (relational::statement_column (table, r, type, m));
          return true;
        }
      };
      entry<view_columns> view_columns_;

      //
      // bind
      //

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
          // While the is_unsigned should indicate whether the
          // buffer variable is unsigned, rather than whether the
          // database type is unsigned, in case of the image types,
          // this is the same.
          //
          os << b << ".buffer_type = " <<
            integer_buffer_types[mi.st->type - sql_type::TINYINT] << ";"
             << b << ".is_unsigned = " << (mi.st->unsign ? "1" : "0") << ";"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_float (member_info& mi)
        {
          os << b << ".buffer_type = " <<
            float_buffer_types[mi.st->type - sql_type::FLOAT] << ";"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          os << b << ".buffer_type = MYSQL_TYPE_NEWDECIMAL;"
             << b << ".buffer = " << arg << "." << mi.var << "value.data ();"
             << b << ".buffer_length = static_cast<unsigned long> (" << endl
             << arg << "." << mi.var << "value.capacity ());"
             << b << ".length = &" << arg << "." << mi.var << "size;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_date_time (member_info& mi)
        {
          os << b << ".buffer_type = " <<
            date_time_buffer_types[mi.st->type - sql_type::DATE] << ";"
             << b << ".buffer = &" << arg << "." << mi.var << "value;";

          if (mi.st->type == sql_type::YEAR)
            os << b << ".is_unsigned = 0;";

          os << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_short_string (member_info& mi)
        {
          // MySQL documentation is quite confusing about the use of
          // buffer_length and length when it comes to input parameters.
          // Source code, however, tells us that it uses buffer_length
          // only if length is NULL.
          //
          os << b << ".buffer_type = " <<
            char_bin_buffer_types[mi.st->type - sql_type::CHAR] << ";"
             << b << ".buffer = " << arg << "." << mi.var << "value.data ();"
             << b << ".buffer_length = static_cast<unsigned long> (" << endl
             << arg << "." << mi.var << "value.capacity ());"
             << b << ".length = &" << arg << "." << mi.var << "size;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << b << ".buffer_type = " <<
            char_bin_buffer_types[mi.st->type - sql_type::CHAR] << ";"
             << b << ".buffer = " << arg << "." << mi.var << "value.data ();"
             << b << ".buffer_length = static_cast<unsigned long> (" << endl
             << arg << "." << mi.var << "value.capacity ());"
             << b << ".length = &" << arg << "." << mi.var << "size;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_bit (member_info& mi)
        {
          // Treated as a BLOB.
          //
          os << b << ".buffer_type = MYSQL_TYPE_BLOB;"
             << b << ".buffer = " << arg << "." << mi.var << "value;"
             << b << ".buffer_length = static_cast<unsigned long> (" << endl
             << "sizeof (" << arg << "." << mi.var << "value));"
             << b << ".length = &" << arg << "." << mi.var << "size;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_enum (member_info& mi)
        {
          // Represented as either integer or string.
          //
          os << "mysql::enum_traits::bind (" << b << "," << endl
             << arg << "." << mi.var << "value," << endl
             << arg << "." << mi.var << "size," << endl
             << "&" << arg << "." << mi.var << "null);";
        }

        virtual void
        traverse_set (member_info& mi)
        {
          // Represented as a string.
          //
          os << b << ".buffer_type = MYSQL_TYPE_STRING;"
             << b << ".buffer = " << arg << "." << mi.var << "value.data ();"
             << b << ".buffer_length = static_cast<unsigned long> (" << endl
             << arg << "." << mi.var << "value.capacity ());"
             << b << ".length = &" << arg << "." << mi.var << "size;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }
      };
      entry<bind_member> bind_member_;

      //
      // grow
      //

      struct grow_member: relational::grow_member_impl<sql_type>,
                          member_base
      {
        grow_member (base const& x)
            : member_base::base (x),      // virtual base
              member_base::base_impl (x), // virtual base
              base_impl (x),
              member_base (x) {}

        virtual void
        traverse_integer (member_info&)
        {
          os << e << " = 0;"
             << endl;
        }

        virtual void
        traverse_float (member_info&)
        {
          os << e << " = 0;"
             << endl;
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          // @@ Optimization disabled.
          //
          os << "if (" << e << ")" << endl
             << "{"
             << "i." << mi.var << "value.capacity (i." << mi.var << "size);"
             << "grew = true;"
             << "}";
        }

        virtual void
        traverse_date_time (member_info&)
        {
          os << e << " = 0;"
             << endl;
        }

        virtual void
        traverse_short_string (member_info& mi)
        {
          // @@ Optimization disabled.
          //
          os << "if (" << e << ")" << endl
             << "{"
             << "i." << mi.var << "value.capacity (i." << mi.var << "size);"
             << "grew = true;"
             << "}";
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << "if (" << e << ")" << endl
             << "{"
             << "i." << mi.var << "value.capacity (i." << mi.var << "size);"
             << "grew = true;"
             << "}";
        }

        virtual void
        traverse_bit (member_info&)
        {
          os << e << " = 0;"
             << endl;
        }

        virtual void
        traverse_enum (member_info& mi)
        {
          // Represented as either integer or string (and we don't know
          // at the code generation time which one it is).
          //
          os << "if (" << e << ")" << endl
             << "{"
             << "if (mysql::enum_traits::grow (" <<
            "i." << mi.var << "value, " <<
            "i." << mi.var << "size))" << endl
             << "grew = true;" // String
             << "else" << endl
             << e << " = 0;" // Integer.
             << "}";
        }

        virtual void
        traverse_set (member_info& mi)
        {
          // Represented as a string.
          //
          os << "if (" << e << ")" << endl
             << "{"
             << "i." << mi.var << "value.capacity (i." << mi.var << "size);"
             << "grew = true;"
             << "}";
        }
      };
      entry<grow_member> grow_member_;

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
          os << "i." << mi.var << "null = 1;";
        }

        virtual void
        traverse_integer (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "null = is_null;";
        }

        virtual void
        traverse_float (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "null = is_null;";
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          // @@ Optimization: can remove growth check if buffer is fixed.
          //
          os << "std::size_t size (0);"
             << "std::size_t cap (i." << mi.var << "value.capacity ());"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;"
             << "i." << mi.var << "size = static_cast<unsigned long> (size);"
             << "grew = grew || (cap != i." << mi.var << "value.capacity ());";
        }

        virtual void
        traverse_date_time (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value, is_null, " << member << ");"
             << "i." << mi.var << "null = is_null;";
        }

        virtual void
        traverse_short_string (member_info& mi)
        {
          // @@ Optimization: can remove growth check if buffer is fixed.
          //
          os << "std::size_t size (0);"
             << "std::size_t cap (i." << mi.var << "value.capacity ());"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;"
             << "i." << mi.var << "size = static_cast<unsigned long> (size);"
             << "grew = grew || (cap != i." << mi.var << "value.capacity ());";
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << "std::size_t size (0);"
             << "std::size_t cap (i." << mi.var << "value.capacity ());"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;"
             << "i." << mi.var << "size = static_cast<unsigned long> (size);"
             << "grew = grew || (cap != i." << mi.var << "value.capacity ());";
        }

        virtual void
        traverse_bit (member_info& mi)
        {
          // Represented as a BLOB.
          //
          os << "std::size_t size (0);"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "sizeof (i." << mi.var << "value)," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;"
             << "i." << mi.var << "size = static_cast<unsigned long> (size);";
        }

        virtual void
        traverse_enum (member_info& mi)
        {
          // Represented as either integer or string.
          //
          os << "if (mysql::enum_traits::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "is_null," << endl
             << member << "))" << endl
             << "grew = true;"
             << endl
             << "i." << mi.var << "null = is_null;";
        }

        virtual void
        traverse_set (member_info& mi)
        {
          // Represented as a string.
          //
          os << "std::size_t size (0);"
             << "std::size_t cap (i." << mi.var << "value.capacity ());"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;"
             << "i." << mi.var << "size = static_cast<unsigned long> (size);"
             << "grew = grew || (cap != i." << mi.var << "value.capacity ());";
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
          os << "i." << var << "null";
        }

        virtual void
        traverse_integer (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_float (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_date_time (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_short_string (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_bit (member_info& mi)
        {
          // Represented as a BLOB.
          //
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_enum (member_info& mi)
        {
          // Represented as either integer or string.
          //
          os << "mysql::enum_traits::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_set (member_info& mi)
        {
          // Represented as a string.
          //
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }
      };
      entry<init_value_member> init_value_member_;

      struct class_: relational::class_, context
      {
        class_ (base const& x): base (x) {}

        virtual void
        init_auto_id (semantics::data_member& m, string const& im)
        {
          // Don't set the id value to 0 if this is a nullable wrapper. This
          // will allow the user to use the NO_AUTO_VALUE_ON_ZERO mode by
          // making it NULL when they want the auto semantics:
          //
          // #pragma db auto
          // odb::nullable<int64_t> id;
          //
          semantics::type& t (utype (m));
          if (wrapper (t) && t.template get<bool> ("wrapper-null-handler"))
            return;

          os << im << "value = 0;"
             << endl;
        }

        virtual string
        join_syntax (view_object const& vo)
        {
          if (vo.join == view_object::full)
          {
            error (vo.loc)
              << "FULL OUTER JOIN is not supported by MySQL" << endl;
            throw operation_failed ();
          }

          return base::join_syntax (vo);
        }
      };
      entry<class_> class_entry_;

      struct include: relational::include, context
      {
        include (base const& x): base (x) {}

        virtual void
        extra_post ()
        {
          os << "#include <odb/mysql/enum.hxx>" << endl;
        }
      };
      entry<include> include_;
    }
  }
}
