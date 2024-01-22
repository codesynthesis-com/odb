// file      : odb/relational/oracle/model.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <sstream>

#include <odb/relational/model.hxx>

#include <odb/relational/oracle/common.hxx>
#include <odb/relational/oracle/context.hxx>

using namespace std;

namespace relational
{
  namespace oracle
  {
    namespace model
    {
      namespace relational = relational::model;

      struct object_columns: relational::object_columns, context
      {
        object_columns (base const& x): base (x) {}

        virtual string
        default_enum (semantics::data_member& m, tree en, string const&)
        {
          // Make sure the column is mapped to Oracle NUMBER.
          //
          sql_type const& t (parse_sql_type (column_type (), m, false));
          if (t.type != sql_type::NUMBER)
          {
            cerr << m.file () << ":" << m.line () << ":" << m.column ()
                 << ": error: column with default value specified as C++ "
                 << "enumerator must map to Oracle NUMBER" << endl;

            throw operation_failed ();
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

        virtual void
        primary_key (sema_rel::primary_key& pk)
        {
          if (pk.auto_ ())
            pk.extra ()["sequence"] = sequence_name (table_.name ()).string ();
        }
      };
      entry<object_columns> object_columns_;
    }
  }
}
