// file      : odb/relational/pgsql/header.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/header.hxx>

#include <odb/relational/pgsql/common.hxx>
#include <odb/relational/pgsql/context.hxx>

namespace relational
{
  namespace pgsql
  {
    namespace header
    {
      namespace relational = relational::header;

      struct class1: relational::class1
      {
        class1 (base const& x): base (x) {}

        virtual void
        object_public_extra_post (type& c)
        {
          bool abst (abstract (c));

          type* poly_root (polymorphic (c));
          bool poly (poly_root != 0);
          bool poly_derived (poly && poly_root != &c);

          if (abst && !poly)
            return;

          data_member_path* id (id_member (c));
          semantics::data_member* optimistic (context::optimistic (c));

          column_count_type const& cc (column_count (c));

          size_t update_columns (
            cc.total - cc.id - cc.inverse - cc.readonly - cc.separate_update);

          // Statement names.
          //
          os << "static const char persist_statement_name[];";

          if (id != 0)
          {
            if (poly_derived)
              os << "static const char* const find_statement_names[" <<
                (abst ? "1" : "depth") << "];";
            else
              os << "static const char find_statement_name[];";

            if (poly && !poly_derived)
              os << "static const char find_discriminator_statement_name[];";

            if (update_columns != 0)
              os << "static const char update_statement_name[];";

            os << "static const char erase_statement_name[];";

            if (optimistic != 0)
              os << "static const char optimistic_erase_statement_name[];";
          }

          // Query statement name.
          //
          if (options.generate_query ())
            os << "static const char query_statement_name[];"
               << "static const char erase_query_statement_name[];";

          os << endl;

          // Statement types.
          //
          os << "static const unsigned int persist_statement_types[];";

          if (id != 0)
          {
            os << "static const unsigned int find_statement_types[];";

            if (update_columns != 0)
              os << "static const unsigned int update_statement_types[];";

            if (optimistic != 0)
              os << "static const unsigned int " <<
                "optimistic_erase_statement_types[];";
          }

          os << endl;

          if (poly_derived)
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
        }

        virtual void
        view_public_extra_post (type&)
        {
          // Statement names.
          //
          os << "static const char query_statement_name[];"
             << endl;
        }
      };
      entry<class1> class1_entry_;

      struct container_traits: relational::container_traits, context
      {
        container_traits (base const& x): base (x) {}

        virtual void
        container_public_extra_pre (semantics::data_member& m,
                                    semantics::type& t)
        {
          if (!object (c_) || (abstract (c_) && !polymorphic (c_)))
            return;

          bool smart (!inverse (m, "value") && !unordered (m) &&
                      container_smart (t));

          // Container statement names.
          //
          os << "static const char select_name[];"
             << "static const char insert_name[];";

          if (smart)
            os << "static const char update_name[];";

          os << "static const char delete_name[];"
             << endl;

          // Container statement types.
          //
          os << "static const unsigned int insert_types[];";

          if (smart)
            os << "static const unsigned int update_types[];"
               << "static const unsigned int delete_types[];";

          os << endl;
        }
      };
      entry<container_traits> container_traits_;

      struct section_traits: relational::section_traits, context
      {
        section_traits (base const& x): base (x) {}

        virtual void
        section_public_extra_post (user_section& s)
        {
          semantics::class_* poly_root (polymorphic (c_));
          bool poly (poly_root != 0);

          if (!poly && (abstract (c_) ||
                        s.special == user_section::special_version))
            return;

          bool load (s.total != 0 && s.separate_load ());
          bool load_opt (s.optimistic () && s.separate_load ());

          bool update (s.total != s.inverse + s.readonly); // Always separate.
          bool update_opt (s.optimistic () && (s.readwrite_containers || poly));

          // Statement names.
          //
          if (load || load_opt)
            os << "static const char select_name[];"
               << endl;

          if (update || update_opt)
            os << "static const char update_name[];"
               << endl;

          // Statement types.
          //
          if (update || update_opt)
            os << "static const unsigned int update_types[];";
        }
      };
      entry<section_traits> section_traits_;

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
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_float (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_numeric (member_info& mi)
        {
          // Exchanged as strings. Can have up to 1000 digits not counting
          // '-' and '.'.
          //

          os << image_type << " " << mi.var << "value;"
             << "std::size_t " << mi.var << "size;"
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_date_time (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_string (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "std::size_t " << mi.var << "size;"
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_bit (member_info& mi)
        {
          // Additional 4 bytes at the beginning of the array specify
          // the number of significant bits in the image. This number
          // is stored in network byte order.
          //
          unsigned int n (4 + mi.st->range / 8 + (mi.st->range % 8 ? 1 : 0));

          os << "unsigned char " << mi.var << "value[" << n << "];"
             << "std::size_t " << mi.var << "size;"
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_varbit (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "std::size_t " << mi.var << "size;"
             << "bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_uuid (member_info& mi)
        {
          // UUID is a 16-byte sequence.
          //
          os << "unsigned char " << mi.var << "value[16];"
             << "bool " << mi.var << "null;"
             << endl;
        }
      };
      entry<image_member> image_member_;
    }
  }
}
