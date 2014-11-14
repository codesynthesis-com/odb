// file      : odb/relational/oracle/header.cxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/header.hxx>

#include <odb/relational/oracle/common.hxx>
#include <odb/relational/oracle/context.hxx>

namespace relational
{
  namespace oracle
  {
    namespace header
    {
      namespace relational = relational::header;

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
                os << "oracle::change_callback change_callback_;"
                   << endl;

              os << "oracle::change_callback*" << endl
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

      struct image_member: relational::image_member, member_base
      {
        image_member (base const& x)
            : member_base::base (x), // virtual base
              base (x),
              member_base (x),
              member_image_type_ (base::type_override_,
                                  base::fq_type_override_,
                                  base::key_prefix_)
        {
        }

        virtual bool
        pre (member_info& mi)
        {
          if (container (mi))
            return false;

          image_type = member_image_type_.image_type (mi.m);

          if (var_override_.empty ())
            os << "// " << mi.m.name () << endl
               << "//" << endl;

          return true;
        }

        virtual void
        traverse_composite (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << endl;
        }

        virtual void
        traverse_int32 (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_int64 (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_big_int (member_info& mi)
        {
          // Each significant base-100 digit requires a byte of storage
          // in the manitissa. The default precision is 38 decimal digits,
          // which is equivalent to 19 base-100 digits.
          //
          size_t n (19);

          if (mi.st->prec)
            n = mi.st->prec_value / 2 + mi.st->prec_value % 2;

          // We require an additional byte for each of the exponent and
          // negative value terminator values.
          //
          n += 2;

          os << "char " << mi.var << "value[" << n << "];"
             << "ub2 " << mi.var << "size;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_float (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_double (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_big_float (member_info& mi)
        {
          // big_float is mapped to the OCI type SQLT_NUM, which requires 21
          // bytes of storage.
          //
          os << "char " << mi.var << "value[21];"
             << "ub2 " << mi.var << "size;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_date (member_info& mi)
        {
          os << "char " << mi.var << "value[7];"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_timestamp (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_interval_ym (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_interval_ds (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_string (member_info& mi)
        {
          size_t n (mi.st->prec ? mi.st->prec_value : 1);

          // National characters can be either UTF-8 or UTF-16 encoded,
          // both of which have a maximum character encoding size of 4
          // bytes. Database character set can also be UTF-8 so if the
          // size is specified in characters, then conservatively assume
          // each character can take up to 4 bytes.
          //
          if (!mi.st->byte_semantics) // N*CHAR always has CHAR semantics.
            n *= 4;

          if (mi.st->type == sql_type::VARCHAR2 ||
              mi.st->type == sql_type::NVARCHAR2)
            n = n > 4000 ? 4000 : n;
          else
            n = n > 2000 ? 2000 : n;

          os << "char " << mi.var << "value[" << n << "];"
             << "ub2 " << mi.var << "size;"
             << "sb2 " << mi.var << "indicator;"
             << endl;
        }

        virtual void
        traverse_lob (member_info& mi)
        {
          os << "mutable " << image_type << " " << mi.var << "callback;"
             << "sb2 " << mi.var << "indicator;"
             << "oracle::lob " << mi.var << "lob;"
             << endl;
        }

      private:
        string image_type;

        member_image_type member_image_type_;
      };
      entry<image_member> image_member_;

      struct class1: relational::class1
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

          os << "static const std::size_t batch = 1UL;"
             << endl;
        }
      };
      entry<class1> class1_entry_;
    }
  }
}
