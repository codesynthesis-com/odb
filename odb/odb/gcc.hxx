// file      : odb/gcc.hxx
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_GCC_HXX
#define ODB_GCC_HXX

#include <odb/gcc-fwd.hxx>

// Actually, let's keep it out. With it included we can compile in C++98
// but not in C++14 (GCC 6 default).
//
// #if BUILDING_GCC_MAJOR >= 6
// #  include <safe-ctype.h> // See gcc-fwd.hxx.
// #endif

// GCC header includes to get the plugin and parse tree declarations.
// The order is important and doesn't follow any kind of logic.
//

#include <stdlib.h>
#include <gmp.h>

#include <cstdlib> // Include before GCC poisons some declarations.

// GCC 4.7 can be built using either C or C++ compiler. From 4.8 it
// is always built as C++.
//
#if BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR <= 6
#  define ODB_GCC_PLUGIN_C
#elif BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR <= 7
#  include <config.h>
#  ifndef ENABLE_BUILD_WITH_CXX
#    define ODB_GCC_PLUGIN_C
#  endif
#endif

#ifdef ODB_GCC_PLUGIN_C
extern "C"
{
#endif

// GCC's system.h below includes safe-ctype.h which "disables" versions
// from ctype.h. Well, now it's gonna learn how it feels to be disabled.
//
#define SAFE_CTYPE_H

#include <gcc-plugin.h>

#include <config.h>
#include <system.h>
#include <coretypes.h>
#include <tree.h>
#include <real.h>

#include <cpplib.h>
#include <cp/cp-tree.h>

#if BUILDING_GCC_MAJOR > 4 || BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR > 5
#  include <c-family/c-common.h>
#  include <c-family/c-pragma.h>
#else
#  include <c-common.h>
#  include <c-pragma.h>
#endif

#if BUILDING_GCC_MAJOR > 4 || BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR > 8
#  include <stringpool.h> // get_identifier
#endif

#include <diagnostic.h>
#include <output.h>

#ifdef ODB_GCC_PLUGIN_C
} // extern "C"
#endif

// Get the value of INTEGER_CST reinterpreted as unsigned.
//
inline unsigned long long
integer_value (tree n)
{
  unsigned long long val;

#if BUILDING_GCC_MAJOR >= 5
  if (tree_fits_uhwi_p (n))
    val = static_cast<unsigned long long> (tree_to_uhwi (n));
  else
    val = static_cast<unsigned long long> (tree_to_shwi (n));
#else
  HOST_WIDE_INT hwl (TREE_INT_CST_LOW (n));
  HOST_WIDE_INT hwh (TREE_INT_CST_HIGH (n));
  unsigned short width (HOST_BITS_PER_WIDE_INT);

  if (hwh == 0)
    val = static_cast<unsigned long long> (hwl);
  else if (hwh == -1 && hwl != 0)
    val = static_cast<unsigned long long> (hwl);
  else
    val = static_cast<unsigned long long> ((hwh << width) + hwl);
#endif

  return val;
}

// Since 4.7.0 the location may point inside a macro rather than at
// the expansion point. We are only really interested in the expansion
// points so we use the real_source_location() wrapper rather than
// DECL_SOURCE_LOCATION() to do this at the source.
//
inline location_t
real_source_location (tree n)
{
  location_t l (DECL_SOURCE_LOCATION (n));

#if BUILDING_GCC_MAJOR > 4 || BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR > 6
  l = linemap_resolve_location (line_table, l, LRK_MACRO_EXPANSION_POINT, 0);
#endif

  return l;
}

// In 4.9.0 the tree code type was changed from int to enum tree_code.
// the tree_code_name array is also gone with the get_tree_code_name()
// function in its place.
//
#if BUILDING_GCC_MAJOR > 4 || BUILDING_GCC_MAJOR == 4 && BUILDING_GCC_MINOR > 8
typedef enum tree_code gcc_tree_code_type;

inline const char*
gcc_tree_code_name (gcc_tree_code_type tc) {return get_tree_code_name (tc);}
#else
typedef int gcc_tree_code_type;

inline const char*
gcc_tree_code_name (gcc_tree_code_type tc) {return tree_code_name[tc];}
#endif

// Only since GCC 4.7.0.
//
#ifndef LOCATION_COLUMN
#define LOCATION_COLUMN(LOC) (expand_location (LOC).column)
#endif

#ifndef DECL_SOURCE_COLUMN
#define DECL_SOURCE_COLUMN(NODE) LOCATION_COLUMN (DECL_SOURCE_LOCATION (NODE))
#endif

// Only since GCC 4.6.0.
//
#ifndef DECL_CHAIN
#define DECL_CHAIN(x) TREE_CHAIN(x)
#endif

// In GCC 6  ANON_AGGRNAME_P became anon_aggrname_p().
// In GCC 10 anon_aggrname_p() became IDENTIFIER_ANON_P.
//
#if BUILDING_GCC_MAJOR < 6
#  define IDENTIFIER_ANON_P(X) ANON_AGGRNAME_P(X)
#elif BUILDING_GCC_MAJOR < 10
#  define IDENTIFIER_ANON_P(X) anon_aggrname_p(X)
#endif

// In GCC 9:
//
// INCLUDED_FROM     Became linemap_included_from_linemap().
//
// LAST_SOURCE_LINE  Was removed apparently as no longer used. Studying
//                   the line-map.h diff from 8.3 suggests that the old
//                   implementation should still work.
//
#if BUILDING_GCC_MAJOR >= 9

inline const line_map_ordinary*
INCLUDED_FROM (line_maps* set, const line_map_ordinary* map)
{
  return linemap_included_from_linemap (set, map);
}

inline source_location
LAST_SOURCE_LINE_LOCATION (const line_map_ordinary* map)
{
  return (((map[1].start_location - 1
	    - map->start_location)
	   & ~((1 << map->m_column_and_range_bits) - 1))
	  + map->start_location);
}

inline linenum_type
LAST_SOURCE_LINE (const line_map_ordinary* map)
{
  return SOURCE_LINE (map, LAST_SOURCE_LINE_LOCATION (map));
}

#endif

// In GCC 11:
//
// lookup_qualified_name() has a new interface.
//
// DECL_IS_BUILTIN became DECL_IS_UNDECLARED_BUILTIN.
//
#if BUILDING_GCC_MAJOR >= 11

inline tree
lookup_qualified_name (tree scope, tree name, bool type, bool complain)
{
  return lookup_qualified_name (
    scope, name, (type ? LOOK_want::TYPE : LOOK_want::NORMAL), complain);
}

#define DECL_IS_BUILTIN(decl) DECL_IS_UNDECLARED_BUILTIN(decl)

#endif

#endif // ODB_GCC_HXX
