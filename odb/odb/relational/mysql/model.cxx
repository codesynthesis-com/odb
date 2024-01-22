// file      : odb/relational/mysql/model.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <sstream>

#include <odb/relational/model.hxx>
#include <odb/relational/mysql/context.hxx>

using namespace std;

namespace relational
{
  namespace mysql
  {
    namespace model
    {
      namespace relational = relational::model;

      struct object_columns: relational::object_columns, context
      {
        object_columns (base const& x): base (x) {}

        virtual string
        default_bool (semantics::data_member&, bool v)
        {
          // MySQL has TRUE and FALSE as just aliases for 1 and 0. Still
          // use them for self-documentation.
          //
          return v ? "TRUE" : "FALSE";
        }

        virtual string
        default_enum (semantics::data_member& m, tree en, string const& name)
        {
          // Make sure the column is mapped to an ENUM or integer type.
          //
          sql_type const& t (parse_sql_type (column_type (), m, false));

          switch (t.type)
          {
          case sql_type::ENUM:
          case sql_type::TINYINT:
          case sql_type::SMALLINT:
          case sql_type::MEDIUMINT:
          case sql_type::INT:
          case sql_type::BIGINT:
            break;
          default:
            {
              cerr << m.file () << ":" << m.line () << ":" << m.column ()
                   << ": error: column with default value specified as C++ "
                   << "enumerator must map to MySQL ENUM or integer type"
                   << endl;

              throw operation_failed ();
            }
          }

          using semantics::enum_;
          using semantics::enumerator;

          enumerator& er (dynamic_cast<enumerator&> (*unit.find (en)));
          enum_& e (er.enum_ ());

          if (t.type == sql_type::ENUM)
          {
            // Assuming the enumerators in the C++ enum and MySQL ENUM are
            // in the same order, calculate the poistion of the C++
            // enumerator and use that as an index in the MySQL ENUM.
            //
            size_t pos (0);

            for (enum_::enumerates_iterator i (e.enumerates_begin ()),
                   end (e.enumerates_end ()); i != end; ++i)
            {
              if (&i->enumerator () == &er)
                break;

              pos++;
            }

            if (pos < t.enumerators.size ())
              return t.enumerators[pos];
            else
            {
              cerr << m.file () << ":" << m.line () << ":" << m.column ()
                   << ": error: unable to map C++ enumerator '" << name
                   << "' to MySQL ENUM value" << endl;

              throw operation_failed ();
            }
          }
          else
          {
            ostringstream ostr;

            if (e.unsigned_ ())
              ostr << er.value ();
            else
              ostr << static_cast<long long> (er.value ());

            return ostr.str ();
          }
        }
      };
      entry<object_columns> object_columns_;

      struct member_create: relational::member_create, context
      {
        member_create (base const& x): base (x) {}

        virtual string
        table_options (semantics::data_member& m, semantics::type& c)
        {
          string r (relational::member_create::table_options (m, c));

          string const& engine (options.mysql_engine ());
          if (engine != "default")
          {
            // Note: MySQL table options can be separated with spaces.
            //
            if (!r.empty ())
              r += ' ';

            r += "ENGINE=";
            r += engine;
          }

          return r;
        }
      };
      entry<member_create> member_create_;

      struct class_: relational::class_, context
      {
        class_ (base const& x): base (x) {}

        virtual string
        table_options (type& c)
        {
          string r (relational::class_::table_options (c));

          string const& engine (options.mysql_engine ());
          if (engine != "default")
          {
            // Note: MySQL table options can be separated with spaces.
            //
            if (!r.empty ())
              r += ' ';

            r += "ENGINE=";
            r += engine;
          }

          return r;
        }
      };
      entry<class_> class__;
    }
  }
}
