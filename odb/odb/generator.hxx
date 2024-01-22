// file      : odb/generator.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_GENERATOR_HXX
#define ODB_GENERATOR_HXX

#include <vector>

#include <odb/options.hxx>
#include <odb/features.hxx>
#include <odb/semantics/unit.hxx>

class generator_failed {};

void
generate (options const&,
          features&,
          semantics::unit&,
          semantics::path const& file,
          std::vector<semantics::path> const& inputs);

#endif // ODB_GENERATOR_HXX
