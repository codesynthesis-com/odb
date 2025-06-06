// file      : odb/oracle/query-dynamic.cxx
// license   : ODB NCUEL; see accompanying LICENSE file

#include <cstddef> // std::size_t

#include <odb/oracle/query-dynamic.hxx>
#include <odb/oracle/exceptions.hxx>    // lob_comparison

using namespace std;

namespace odb
{
  namespace oracle
  {
    static const char* logic_operators[] = {") AND (", ") OR ("};
    static const char* comp_operators[] = {"=", "!=", "<", ">", "<=", ">="};

    static void
    translate (query_base& q, const odb::query_base& s, size_t p)
    {
      typedef odb::query_base::clause_part part;

      const part& x (s.clause ()[p]);

      switch (x.kind)
      {
      case part::kind_column:
        {
          const query_column_base* c (
            static_cast<const query_column_base*> (
              x.native_info[id_oracle].column));

          q.append (c->table (), c->column ());
          break;
        }
      case part::kind_param_val:
      case part::kind_param_ref:
        {
          const query_column_base& qc (
            *static_cast<const query_column_base*> (
              x.native_info[id_oracle].column));

          query_param_factory f (
            reinterpret_cast<query_param_factory> (
              x.native_info[id_oracle].param_factory));

          // If factory is NULL, then this is a LOB value.
          //
          if (f == 0)
            throw lob_comparison ();

          const odb::query_param* p (
            reinterpret_cast<const odb::query_param*> (x.data));

          q.append (f (p->value, qc, x.kind == part::kind_param_ref),
                    qc.conversion ());
          break;
        }
      case part::kind_native:
        {
          q.append (s.strings ()[x.data]);
          break;
        }
      case part::kind_true:
      case part::kind_false:
        {
          q.append (x.kind == part::kind_true);
          break;
        }
      case part::op_add:
        {
          translate (q, s, x.data);
          translate (q, s, p - 1);
          break;
        }
      case part::op_and:
      case part::op_or:
        {
          q += "(";
          translate (q, s, x.data);
          q += logic_operators[x.kind - part::op_and];
          translate (q, s, p - 1);
          q += ")";
          break;
        }
      case part::op_not:
        {
          q += "NOT (";
          translate (q, s, p - 1);
          q += ")";
          break;
        }
      case part::op_null:
      case part::op_not_null:
        {
          translate (q, s, p - 1);
          q += (x.kind == part::op_null ? "IS NULL" : "IS NOT NULL");
          break;
        }
      case part::op_in:
        {
          if (x.data != 0)
          {
            size_t b (p- x.data);

            translate (q, s, b - 1); // column
            q += "IN (";

            for (size_t i (b); i != p; ++i)
            {
              if (i != b)
                q += ",";

              translate (q, s, i);
            }

            q += ")";
          }
          else
            q.append (false);

          break;
        }
      case part::op_like:
        {
          translate (q, s, p - 2); // column
          q += "LIKE";
          translate (q, s, p - 1); // pattern
          break;
        }
      case part::op_like_escape:
        {
          translate (q, s, p - 3); // column
          q += "LIKE";
          translate (q, s, p - 2); // pattern
          q += "ESCAPE";
          translate (q, s, p - 1); // escape
          break;
        }
      case part::op_eq:
      case part::op_ne:
      case part::op_lt:
      case part::op_gt:
      case part::op_le:
      case part::op_ge:
        {
          translate (q, s, x.data);
          q += comp_operators[x.kind - part::op_eq];
          translate (q, s, p - 1);
          break;
        }
      }
    }

    query_base::
    query_base (const odb::query_base& q)
        : binding_ (0, 0)
    {
      if (!q.empty ())
        translate (*this, q, q.clause ().size () - 1);
    }
  }
}
