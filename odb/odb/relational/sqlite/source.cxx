// file      : odb/relational/sqlite/source.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/source.hxx>

#include <odb/relational/sqlite/common.hxx>
#include <odb/relational/sqlite/context.hxx>

using namespace std;

namespace relational
{
  namespace sqlite
  {
    namespace source
    {
      namespace relational = relational::source;

      struct query_parameters: relational::query_parameters, context
      {
        query_parameters (base const& x): base (x) {}

        virtual string
        next (semantics::data_member& m,
              const string& column,
              const string& sqlt)
        {
          // Handle stream columns. Specifically, we somehow need to
          // pass the column name to the code that runs in the
          // statement. So what we are going to do is encode it
          // in the parameter name.
          //
          if (sk_ == statement_insert || sk_ == statement_update)
          {
            const sql_type& t (parse_sql_type (sqlt, m, false));
            if (t.stream)
            {
              // The column name is quoted.
              //
              string r (column);
              r[0] = '$';               // Replace leading '"'.
              r.resize (r.size () - 1); // Remove trailing '"'.

              // Verify it only contains allowed characters.
              //
              for (size_t i (1); i != r.size (); ++i)
              {
                char c (r[i]);
                if (c != '_' &&
                    (c < '0' || c > '9') &&
                    (c < 'a' || c > 'z') &&
                    (c < 'A' || c > 'Z'))
                {
                  cerr << m.file () << ":" << m.line () << ":" << m.column ()
                       << ": error: unsupported character '" << c << "' in "
                       << sqlt << " column name " << column << endl;

                  cerr << m.file () << ":" << m.line () << ":" << m.column ()
                       << ": info: STREAM column can contain alpha-numeric "
                       << "characters plus '_'" << endl;

                  throw operation_failed ();
                }
              }

              // For TEXT columns, since we use the *_bind_zeroblob()
              // function (there is no *_bind_zerotext()), the value
              // that will be stored is BLOB, not TEXT, unless we
              // explicitly CAST it. The user better make sure the
              // encoding of raw TEXT data they are going to write
              // matches the database encoding.
              //
              if (t.type == sql_type::TEXT)
                r = "CAST(" + r + " AS TEXT)";

              return r;
            }
          }

          return "?";
        }
      };
      entry<query_parameters> query_parameters_;

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
          os << b << ".type = sqlite::bind::integer;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_real (member_info& mi)
        {
          os << b << ".type = sqlite::bind::real;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_text (member_info& mi)
        {
          os << b << ".type = sqlite::image_traits<" << endl
             << "  " << mi.fq_type () << "," << endl
             << "  sqlite::id_text>::bind_value;"
             << b << ".buffer = " << arg << "." << mi.var << "value.data ();"
             << b << ".size = &" << arg << "." << mi.var << "size;"
             << b << ".capacity = " << arg << "." << mi.var <<
            "value.capacity ();"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_blob (member_info& mi)
        {
          os << b << ".type = sqlite::bind::blob;"
             << b << ".buffer = " << arg << "." << mi.var << "value.data ();"
             << b << ".size = &" << arg << "." << mi.var << "size;"
             << b << ".capacity = " << arg << "." << mi.var <<
            "value.capacity ();"
             << b << ".is_null = &" << arg << "." << mi.var << "null;";
        }

        virtual void
        traverse_stream (member_info& mi)
        {
          os << b << ".type = sqlite::bind::stream;"
             << b << ".buffer = &" << arg << "." << mi.var << "value;"
             << b << ".size = &" << arg << "." << mi.var << "size;"
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
          os << e << " = false;"
             << endl;
        }

        virtual void
        traverse_real (member_info&)
        {
          os << e << " = false;"
             << endl;
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << "if (" << e << ")" << endl
             << "{"
             << "i." << mi.var << "value.capacity (i." << mi.var << "size);"
             << "grew = true;"
             << "}";
        }

        virtual void
        traverse_stream (member_info&)
        {
          os << e << " = false;"
             << endl;
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
          os << "i." << mi.var << "null = true;";
        }

        virtual void
        traverse_integer (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;";
        }

        virtual void
        traverse_real (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;";
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << "std::size_t cap (i." << mi.var << "value.capacity ());"
             << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;"
             << "grew = grew || (cap != i." << mi.var << "value.capacity ());";
        }

        virtual void
        traverse_stream (member_info& mi)
        {
          os << traits << "::set_image (" << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "is_null," << endl
             << member << ");"
             << "i." << mi.var << "null = is_null;";
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
        traverse_real (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }

        virtual void
        traverse_stream (member_info& mi)
        {
          os << traits << "::set_value (" << endl
             << member << "," << endl
             << "i." << mi.var << "value," << endl
             << "i." << mi.var << "size," << endl
             << "i." << mi.var << "null);"
             << endl;
        }
      };
      entry<init_value_member> init_value_member_;

      struct statement_columns_common: context
      {
        void
        process (relational::statement_columns& cs, statement_kind sk)
        {
          using relational::statement_columns;

          // For SELECT statements, add _ROWID_ "follow-up" column to
          // each stream column. The reason we need both, and not just
          // ROWID is the NULL value. Let's hope that SELECT'ing a BLOB
          // but not actually reading it with sqlite3_result_blob() is
          // as fast as not SELECT'ing it.
          //
          if (sk != statement_select)
            return;

          for (statement_columns::iterator i (cs.begin ());
               i != cs.end (); ++i)
          {
            if (parse_sql_type (i->type, *i->member).stream)
            {
              // Column is already table-qualified and quoted. Do some
              // surgery to replace it with _ROWID_. That is, we want to
              // transform "table"."column" to "table"."_ROWID_".
              //
              string c (i->column);
              string::size_type n (c.size ()), p (c.rfind ('"', n - 2));
              assert (p != string::npos);
              string as (c, p + 1, n - p - 2);
              c.resize (p);
              c += "\"_ROWID_\"";

              // We are going to pack this "tightly", without any newlines,
              // so that the statement processing code treats them as a
              // single column.
              //
              i->column += ',';
              i->column += c;
            }
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
          // Caching is not necessary since SQLite can execute several
          // interleaving statements.
          //
        }

        virtual void
        process_statement_columns (relational::statement_columns& cols,
                                   statement_kind sk,
                                   bool)
        {
          statement_columns_common::process (cols, sk);
        }
      };
      entry<container_traits> container_traits_;

      struct section_traits: relational::section_traits,
                             statement_columns_common
      {
        section_traits (base const& x): base (x) {}

        virtual void
        process_statement_columns (relational::statement_columns& cols,
                                   statement_kind sk,
                                   bool)
        {
          statement_columns_common::process (cols, sk);
        }
      };
      entry<section_traits> section_traits_;

      struct class_: relational::class_, statement_columns_common
      {
        class_ (base const& x): base (x) {}

        virtual void
        init_auto_id (semantics::data_member& m, string const& im)
        {
          // Don't set the id value to NULL if this is a nullable wrapper.
          // This will allow the user to control whether the value is auto or
          // manually assigned by using something like this:
          //
          // #pragma db auto
          // odb::nullable<int64_t> id;
          //
          semantics::type& t (utype (m));
          if (wrapper (t) && t.template get<bool> ("wrapper-null-handler"))
            return;

          os << im << "null = true;"
             << endl;
        }

        virtual string
        select_trailer (type&)
        {
          // SQLite has not support for FOR UPDATE and since this is an
          // optimization, we simply ignore it.
          //
          return "";
        }

        virtual string
        join_syntax (view_object const& vo)
        {
          const char* n (0);

          if (vo.join == view_object::full)
            n = "FULL OUTER JOIN";
          else if (vo.join == view_object::right)
            n = "RIGHT OUTER JOIN";

          if (n != 0)
          {
            error (vo.loc) << n << " is not supported by SQLite" << endl;
            throw operation_failed ();
          }

          return base::join_syntax (vo);
        }

        virtual void
        process_statement_columns (relational::statement_columns& cols,
                                   statement_kind sk,
                                   bool)
        {
          statement_columns_common::process (cols, sk);
        }
      };
      entry<class_> class_entry_;
    }
  }
}
