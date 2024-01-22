// file      : odb/relational/validator.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_RELATIONAL_VALIDATOR_HXX
#define ODB_RELATIONAL_VALIDATOR_HXX

#include <odb/options.hxx>
#include <odb/features.hxx>
#include <odb/semantics/unit.hxx>

namespace relational
{
  // The first pass is performed before processing. The second -- after.
  // Throws operation_failed to signal a failure.
  //
  void
  validate (options const&,
            features&,
            semantics::unit&,
            semantics::path const&,
            unsigned short pass);
}

#endif // ODB_RELATIONAL_VALIDATOR_HXX
