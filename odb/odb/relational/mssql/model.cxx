// file      : odb/relational/mssql/model.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <sstream>

#include <odb/relational/model.hxx>

#include <odb/relational/mssql/common.hxx>
#include <odb/relational/mssql/context.hxx>

using namespace std;

namespace relational
{
  namespace mssql
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
          // Make sure the column is mapped to an integer or DECIMAL type.
          //
          switch (parse_sql_type (column_type (), m, false).type)
          {
          case sql_type::BIT:
          case sql_type::TINYINT:
          case sql_type::SMALLINT:
          case sql_type::INT:
          case sql_type::BIGINT:
          case sql_type::DECIMAL:
            break;
          default:
            {
              cerr << m.file () << ":" << m.line () << ":" << m.column ()
                   << ": error: column with default value specified as C++ "
                   << "enumerator must map to SQL Server integer type" << endl;

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
