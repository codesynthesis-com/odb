// file      : odb/query-dynamic.ixx
// license   : GNU GPL v2; see accompanying LICENSE file

namespace odb
{
  // query_base
  //
  template <typename B>
  inline query_base::
  query_base (const query_column<bool, B>& c)
  {
    // Some databases provide the IS TRUE operator. However, we cannot
    // use it since the column type might now be SQL boolean type.
    //
    append (c.native_info);
    B::append_val (*this, true, c.native_info);
    append (query_base::clause_part::op_eq, 0);
  }
}
