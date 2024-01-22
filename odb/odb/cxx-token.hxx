// file      : odb/cxx-token.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_CXX_TOKEN_HXX
#define ODB_CXX_TOKEN_HXX

#include <string>
#include <vector>

#include <odb/gcc-fwd.hxx>

struct cxx_token
{
  cxx_token (location_t l,
             unsigned int t,
             std::string const& lt = std::string (),
             tree n = 0)
      : loc (l), type (t), literal (lt), node (n) {}

  location_t loc;      // Location of this token.
  unsigned int type;   // Untyped cpp_ttype.
  std::string literal; // Only used for name, keyword, string, amd number.
  tree node;           // Tree node for the number. The number can be
                       // represented as either literal, tree node, or
                       // both, depending on which lexer was used.
};

typedef std::vector<cxx_token> cxx_tokens;

#endif // ODB_CXX_TOKEN_HXX
