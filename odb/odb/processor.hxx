// file      : odb/processor.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_PROCESSOR_HXX
#define ODB_PROCESSOR_HXX

#include <odb/options.hxx>
#include <odb/features.hxx>
#include <odb/semantics/unit.hxx>

class processor_failed {};

// Pass one is very early processing before any validation has been
// done.
//
void
process (options const&,
         features&,
         semantics::unit&,
         semantics::path const&,
         unsigned short pass);

#endif // ODB_PROCESSOR_HXX
