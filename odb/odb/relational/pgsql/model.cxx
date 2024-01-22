// file      : odb/relational/pgsql/model.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <sstream>

#include <odb/diagnostics.hxx>

#include <odb/relational/model.hxx>

#include <odb/relational/pgsql/common.hxx>
#include <odb/relational/pgsql/context.hxx>

using namespace std;

namespace relational
{
  namespace pgsql
  {
    namespace model
    {
      namespace relational = relational::model;

      struct object_columns: relational::object_columns, context
      {
        object_columns (base const& x): base (x) {}

        virtual void
        traverse_object (semantics::class_& c)
        {
          base::traverse_object (c);

          if (context::top_object == &c)
          {
            // Make sure that the auto id type is INTEGER or BIGINT.
            //
            if (pkey_ != 0 && pkey_->auto_ ())
            {
              // Should be a single column.
              //
              sema_rel::column& c (pkey_->contains_begin ()->column ());

              // This should never fail since we have already parsed this.
              //
              sql_type const& t (parse_sql_type (c.type ()));

              if (t.type != sql_type::INTEGER && t.type != sql_type::BIGINT)
              {
                location const& l (c.get<location> ("cxx-location"));
                error (l) << "automatically assigned object id must map "
                          << "to PostgreSQL INTEGER or BIGINT" << endl;
                throw operation_failed ();
              }
            }
          }
        }

        virtual string
        default_bool (semantics::data_member&, bool v)
        {
          return v ? "TRUE" : "FALSE";
        }

        virtual string
        default_enum (semantics::data_member& m, tree en, string const&)
        {
          // Make sure the column is mapped to an integer type.
          //
          switch (parse_sql_type (column_type (), m, false).type)
          {
          case sql_type::SMALLINT:
          case sql_type::INTEGER:
          case sql_type::BIGINT:
            break;
          default:
            {
              cerr << m.file () << ":" << m.line () << ":" << m.column ()
                   << ": error: column with default value specified as C++ "
                   << "enumerator must map to PostgreSQL integer type" << endl;

              throw operation_failed ();
            }
          }

          using semantics::enumerator;

          enumerator& e (dynamic_cast<enumerator&> (*unit.find (en)));

          ostringstream ostr;

          if (e.enum_ ().unsigned_ ())
            ostr << e.value ();
          else
            ostr << static_cast<long long> (e.value ());

          return ostr.str ();
        }
      };
      entry<object_columns> object_columns_;
    }
  }
}
