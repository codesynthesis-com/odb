// file      : odb/semantics/fundamental.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_SEMANTICS_FUNDAMENTAL_HXX
#define ODB_SEMANTICS_FUNDAMENTAL_HXX

#include <odb/semantics/elements.hxx>

namespace semantics
{
  //
  // Fundamental C++ types.
  //

  struct fund_type: type
  {
    virtual string
    fq_name () const;

    virtual string
    fq_name (names*) const;
  };

  struct fund_void: fund_type
  {
    fund_void (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
  };

  //
  // Integral.
  //

  struct integral_type: fund_type
  {
    virtual bool
    unsigned_ () const = 0;
  };

  struct fund_bool: integral_type
  {
    fund_bool (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_char: integral_type
  {
    fund_char (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const;
  };

  struct fund_wchar: integral_type
  {
    fund_wchar (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const;
  };

  struct fund_char16: integral_type
  {
    fund_char16 (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_char32: integral_type
  {
    fund_char32 (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_signed_char: integral_type
  {
    fund_signed_char (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return false;}
  };

  struct fund_unsigned_char: integral_type
  {
    fund_unsigned_char (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_short: integral_type
  {
    fund_short (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return false;}
  };

  struct fund_unsigned_short: integral_type
  {
    fund_unsigned_short (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_int: integral_type
  {
    fund_int (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return false;}
  };

  struct fund_unsigned_int: integral_type
  {
    fund_unsigned_int (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_long: integral_type
  {
    fund_long (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return false;}
  };

  struct fund_unsigned_long: integral_type
  {
    fund_unsigned_long (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  struct fund_long_long: integral_type
  {
    fund_long_long (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return false;}
  };

  struct fund_unsigned_long_long: integral_type
  {
    fund_unsigned_long_long (tree tn)
        : node (path ("<fundamental>"), 0, 0, tn) {}
    virtual bool unsigned_ () const {return true;}
  };

  //
  // Real.
  //

  struct fund_float: fund_type
  {
    fund_float (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
  };

  struct fund_double: fund_type
  {
    fund_double (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
  };

  struct fund_long_double: fund_type
  {
    fund_long_double (tree tn): node (path ("<fundamental>"), 0, 0, tn) {}
  };
}

#endif // ODB_SEMANTICS_FUNDAMENTAL_HXX
