// file      : odb/features.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_FEATURES_HXX
#define ODB_FEATURES_HXX

#include <cstring> // std::memset

// Optional features used by client code that affect generated code.
//
struct features
{
  features () {std::memset (this, 0, sizeof (features));}

  bool tr1_pointer;
  bool boost_pointer;
  bool simple_object;
  bool polymorphic_object;
  bool no_id_object;
  bool session_object;
  bool section;
  bool view;
};

#endif // ODB_FEATURES_HXX
