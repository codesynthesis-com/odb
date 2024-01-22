// file      : odb/validator.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_VALIDATOR_HXX
#define ODB_VALIDATOR_HXX

#include <odb/options.hxx>
#include <odb/features.hxx>
#include <odb/semantics/unit.hxx>

class validator_failed {};

// The first pass is performed after processing pass 1 but before
// processing pass 2. The second -- after pass 2.
//
void
validate (options const&,
          features&,
          semantics::unit&,
          semantics::path const&,
          unsigned short pass);

#endif // ODB_VALIDATOR_HXX
