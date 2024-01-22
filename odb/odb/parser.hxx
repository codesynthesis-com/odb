// file      : odb/parser.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_PARSER_HXX
#define ODB_PARSER_HXX

#include <odb/gcc.hxx>

#include <memory>  // std::unique_ptr

#include <odb/pragma.hxx>
#include <odb/options.hxx>
#include <odb/semantics/unit.hxx>

class parser
{
public:
  class failed {};

  ~parser ();
  parser (options const&, loc_pragmas&, ns_loc_pragmas&, decl_pragmas&);

  std::unique_ptr<semantics::unit>
  parse (tree global_scope, semantics::path const& main_file);

private:
  parser (parser const&);

  parser&
  operator= (parser const&);

private:
  class impl;
  std::unique_ptr<impl> impl_;
};

#endif // ODB_PARSER_HXX
