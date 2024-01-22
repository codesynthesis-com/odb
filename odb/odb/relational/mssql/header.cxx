// file      : odb/relational/mssql/header.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/header.hxx>

#include <odb/relational/mssql/common.hxx>
#include <odb/relational/mssql/context.hxx>

namespace relational
{
  namespace mssql
  {
    namespace header
    {
      namespace relational = relational::header;

      struct class1: relational::class1, context
      {
        class1 (base const& x): base (x) {}

        virtual void
        object_public_extra_pre (type& c)
        {
          bool abst (abstract (c));

          type* poly_root (polymorphic (c));
          bool poly (poly_root != 0);
          bool poly_derived (poly && poly_root != &c);

          if (poly_derived || (abst && !poly))
            return;

          // Bulk operations batch size.
          //
          {
            unsigned long long b (c.count ("bulk")
                                  ? c.get<unsigned long long> ("bulk")
                                  : 1);

            os << "static const std::size_t batch = " << b << "UL;"
               << endl;
          }

          // rowvesion
          //
          bool rv (false);
          if (semantics::data_member* m = optimistic (c))
          {
            sql_type t (parse_sql_type (column_type (*m), *m));
            rv = (t.type == sql_type::ROWVERSION);
          }

          os << "static const bool rowversion = " << rv << ";"
             << endl;

          // Disable bulk update if we have ROWVERSION since we don't
          // yet support batch extraction of the version.
          //
          if (rv && c.count ("bulk-update"))
            c.remove ("bulk-update");
        }

        virtual void
        object_public_extra_post (type& c)
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
              os << "static version_type" << endl
                 << "version (const id_image_type&);"
                 << endl;
            }
          }
        }
      };
      entry<class1> class1_entry_;

      struct section_traits: relational::section_traits, context
      {
        section_traits (base const& x): base (x) {}

        virtual void
        section_public_extra_pre (user_section&)
        {
          if (abstract (c_) && !polymorphic (c_))
            return;

          // rowvesion
          //
          bool rv (false);
          if (semantics::data_member* m = optimistic (c_))
          {
            sql_type t (parse_sql_type (column_type (*m), *m));
            rv = (t.type == sql_type::ROWVERSION);
          }

          os << "static const bool rowversion = " << rv << ";"
             << endl;
        }
      };
      entry<section_traits> section_traits_;

      struct image_type: relational::image_type, context
      {
        image_type (base const& x): base (x) {};

        virtual void
        image_extra (type& c)
        {
          if (!(composite (c) || (abstract (c) && !polymorphic (c))))
          {
            type* poly_root (polymorphic (c));

            // If this is a polymorphic type, only add callback to the root.
            //
            if (poly_root == 0 || poly_root == &c)
            {
              bool gc (options.generate_query ());

              if (gc)
                os << "mssql::change_callback change_callback_;"
                   << endl;

              os << "mssql::change_callback*" << endl
                 << "change_callback ()"
                 << "{";

              if (gc)
                os << "return &change_callback_;";
              else
                os << "return 0;";

              os << "}";
            }
          }
        }
      };
      entry<image_type> image_type_;

      struct image_member: relational::image_member_impl<sql_type>,
                           member_base
      {
        image_member (base const& x)
            : member_base::base (x),      // virtual base
              member_base::base_impl (x), // virtual base
              base_impl (x),
              member_base (x) {}

        virtual void
        traverse_integer (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_smallmoney (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_money (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_float4 (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_float8 (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_string (member_info& mi)
        {
          // Extra character for the null-terminator that ODBC always adds.
          //
          os << "char " << mi.var << "value[" << mi.st->prec + 1 << "];"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << "mutable " << image_type << " " << mi.var << "callback;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_nstring (member_info& mi)
        {
          // Extra character for the null-terminator that ODBC always adds.
          //
          os << "mssql::ucs2_char " << mi.var << "value[" <<
            mi.st->prec + 1 << "];"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_long_nstring (member_info& mi)
        {
          os << "mutable " << image_type << " " << mi.var << "callback;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_binary (member_info& mi)
        {
          os << "char " << mi.var << "value[" << mi.st->prec << "];"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_long_binary (member_info& mi)
        {
          os << "mutable " << image_type << " " << mi.var << "callback;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_date (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_time (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_datetime (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_datetimeoffset (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_uniqueidentifier (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }

        virtual void
        traverse_rowversion (member_info& mi)
        {
          os << "unsigned char " << mi.var << "value[8];"
             << "SQLLEN " << mi.var << "size_ind;"
             << endl;
        }
      };
      entry<image_member> image_member_;
    }
  }
}
