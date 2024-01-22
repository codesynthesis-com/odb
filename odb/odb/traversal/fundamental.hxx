// file      : odb/traversal/fundamental.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_TRAVERSAL_FUNDAMENTAL_HXX
#define ODB_TRAVERSAL_FUNDAMENTAL_HXX

#include <odb/semantics/fundamental.hxx>
#include <odb/traversal/elements.hxx>

namespace traversal
{
  struct fund_type: node<semantics::fund_type> {};

  struct fund_void: node<semantics::fund_void> {};
  struct fund_bool: node<semantics::fund_bool> {};

  // Integral.
  //
  struct fund_char: node<semantics::fund_char> {};
  struct fund_wchar: node<semantics::fund_wchar> {};
  struct fund_signed_char: node<semantics::fund_signed_char> {};
  struct fund_unsigned_char: node<semantics::fund_unsigned_char> {};
  struct fund_short: node<semantics::fund_short> {};
  struct fund_unsigned_short: node<semantics::fund_unsigned_short> {};
  struct fund_int: node<semantics::fund_int> {};
  struct fund_unsigned_int: node<semantics::fund_unsigned_int> {};
  struct fund_long: node<semantics::fund_long> {};
  struct fund_unsigned_long: node<semantics::fund_unsigned_long> {};
  struct fund_long_long: node<semantics::fund_long_long> {};
  struct fund_unsigned_long_long: node<semantics::fund_unsigned_long_long> {};

  // Real.
  //
  struct fund_float: node<semantics::fund_float> {};
  struct fund_double: node<semantics::fund_double> {};
  struct fund_long_double: node<semantics::fund_long_double> {};
}

#endif // ODB_TRAVERSAL_FUNDAMENTAL_HXX
