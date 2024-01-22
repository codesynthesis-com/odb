// file      : odb/relational/mysql/header.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/relational/header.hxx>

#include <odb/relational/mysql/common.hxx>
#include <odb/relational/mysql/context.hxx>

namespace relational
{
  namespace mysql
  {
    namespace header
    {
      namespace relational = relational::header;

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
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_float (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_decimal (member_info& mi)
        {
          // Exchanged as strings. Can have up to 65 digits not counting
          // '-' and '.'. If range is not specified, the default is 10.
          //

          /*
            @@ Disabled.
            os << "char " << mi.var << "value[" <<
            (t.range ? t.range_value : 10) + 3 << "];"
          */

          os << image_type << " " << mi.var << "value;"
             << "unsigned long " << mi.var << "size;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_date_time (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "my_bool " << mi.var << "null;"
             << endl;

        }

        virtual void
        traverse_short_string (member_info& mi)
        {
          // If range is not specified, the default buffer size is 255.
          //
          /*
            @@ Disabled.
            os << "char " << mi.var << "value[" <<
            (t.range ? t.range_value : 255) + 1 << "];"
          */

          os << image_type << " " << mi.var << "value;"
             << "unsigned long " << mi.var << "size;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_long_string (member_info& mi)
        {
          os << image_type << " " << mi.var << "value;"
             << "unsigned long " << mi.var << "size;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_bit (member_info& mi)
        {
          // Valid range is 1 to 64.
          //
          unsigned int n (mi.st->range / 8 + (mi.st->range % 8 ? 1 : 0));

          os << "unsigned char " << mi.var << "value[" << n << "];"
             << "unsigned long " << mi.var << "size;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_enum (member_info& mi)
        {
          // Represented as either integer or string. Since we don't know
          // at the code generation time which one it is, we have to always
          // keep size in case it is a string.
          //
          os << image_type << " " << mi.var << "value;"
             << "unsigned long " << mi.var << "size;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }

        virtual void
        traverse_set (member_info& mi)
        {
          // Represented as string.
          //
          os << image_type << " " << mi.var << "value;"
             << "unsigned long " << mi.var << "size;"
             << "my_bool " << mi.var << "null;"
             << endl;
        }
      };
      entry<image_member> image_member_;
    }
  }
}
