// file      : common/container/basics/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <common/config.hxx> // HAVE_CXX11

#include <map>
#include <set>
#include <list>
#include <vector>
#include <deque>
#include <string>

#ifdef HAVE_CXX11
#  include <array>
#  include <forward_list>
#  include <unordered_map>
#  include <unordered_set>
#endif

#include <odb/core.hxx>

#pragma db value
struct comp
{
  comp () {}
  comp (int n, const std::string& s) : num (n), str (s) {}

  #pragma db column("number")
  int num;
  std::string str;
};

inline bool
operator== (const comp& x, const comp& y)
{
  return x.num == y.num && x.str == y.str;
}

inline bool
operator!= (const comp& x, const comp& y)
{
  return !(x == y);
}

inline bool
operator< (const comp& x, const comp& y)
{
  return x.num != y.num ? x.num < y.num : x.str < y.str;
}

typedef std::list<std::string> str_list;
typedef std::deque<int> num_deque;

typedef std::vector<int> num_vector;
typedef std::vector<std::string> str_vector;

typedef std::set<int> num_set;
typedef std::set<std::string> str_set;
typedef std::set<comp> comp_set;

typedef std::map<int, std::string> num_str_map;
typedef std::map<std::string, int> str_num_map;
typedef std::map<int, comp> num_comp_map;
typedef std::map<comp, std::string> comp_str_map;

#ifdef HAVE_CXX11
struct comp_hash
{
  std::size_t
  operator() (const comp& x) const {return nh (x.num) + sh (x.str);}

  std::hash<int> nh;
  std::hash<std::string> sh;
};

typedef std::array<int, 3> num_array;
typedef std::array<std::string, 3> str_array;
typedef std::array<comp, 3> comp_array;

typedef std::forward_list<int> num_flist;
typedef std::forward_list<std::string> str_flist;
typedef std::forward_list<comp> comp_flist;

typedef std::unordered_set<int> num_uset;
typedef std::unordered_set<std::string> str_uset;
typedef std::unordered_set<comp, comp_hash> comp_uset;

typedef std::unordered_map<int, std::string> num_str_umap;
typedef std::unordered_map<std::string, int> str_num_umap;
typedef std::unordered_map<int, comp> num_comp_umap;
typedef std::unordered_map<comp, std::string, comp_hash> comp_str_umap;
#endif

#pragma db value
struct cont_comp1
{
  // This composite value does not have any columns.
  //
  num_vector sv; // Have the name "conflic" with the one in the object.
};

#pragma db value
struct cont_comp2
{
  cont_comp2 (): num (777), str ("ggg") {}

  int num;
  str_list sl;
  std::string str;
};

#pragma db object
struct object
{
  object (): nv (comp1_.sv), sl (comp2_.sl) {}
  object (const std::string& id) : id_ (id), nv (comp1_.sv), sl (comp2_.sl) {}

  #pragma db id
  std::string id_;

  int num;

  cont_comp1 comp1_;
  cont_comp2 comp2_;

  // vector
  //
  #pragma db transient
  num_vector& nv;

  #pragma db table("object_strings") id_column ("obj_id")
  str_vector sv;

  #pragma db value_column("")
  std::vector<comp> cv;

  #pragma db unordered
  num_vector uv;

  // list
  //
  #pragma db transient
  str_list& sl;

  // deque
  //
  num_deque nd;

  // set
  //
  num_set ns;
  str_set ss;
  comp_set cs;

  // map
  //
  num_str_map nsm;
  str_num_map snm;
  num_comp_map ncm;
  comp_str_map csm;

#ifdef HAVE_CXX11
  // array
  //
  num_array na;
  str_array sa;
  comp_array ca;

  // forward_list
  //
  num_flist nfl;
  str_flist sfl;
  comp_flist cfl;

  // unordered_set
  //
  num_uset nus;
  str_uset sus;
  comp_uset cus;

  // unordered_map
  //
  num_str_umap nsum;
  str_num_umap snum;
  num_comp_umap ncum;
  comp_str_umap csum;
#else
  // Dummy containers to get the equivalent DROP TABLE statements.
  //
  num_vector na;
  num_vector sa;
  num_vector ca;

  num_vector nfl;
  num_vector sfl;
  num_vector cfl;

  num_set nus;
  str_set sus;
  comp_set cus;

  num_str_map nsum;
  str_num_map snum;
  num_comp_map ncum;
  comp_str_map csum;
#endif

  std::string str;
};

inline bool
operator== (const object& x, const object& y)
{
  if (x.uv.size () != y.uv.size ())
    return false;

  int xs (0), ys (0);

  for (num_vector::size_type i (0); i < x.uv.size (); ++i)
  {
    xs += x.uv[i];
    ys += y.uv[i];
  }

  return
    x.id_ == y.id_ &&
    x.num == y.num &&

    x.comp2_.num == y.comp2_.num &&
    x.comp2_.str == y.comp2_.str &&

    x.nv == y.nv &&
    x.sv == y.sv &&
    x.cv == y.cv &&
    xs == ys &&

    x.sl == y.sl &&

    x.nd == y.nd &&

    x.ns == y.ns &&
    x.ss == y.ss &&
    x.cs == y.cs &&

    x.nsm == y.nsm &&
    x.snm == y.snm &&
    x.ncm == y.ncm &&
    x.csm == y.csm &&

#ifdef HAVE_CXX11
    x.na == y.na &&
    x.sa == y.sa &&
    x.ca == y.ca &&

    x.nfl == y.nfl &&
    x.sfl == y.sfl &&
    x.cfl == y.cfl &&

    x.nus == y.nus &&
    x.sus == y.sus &&
    x.cus == y.cus &&

    x.nsum == y.nsum &&
    x.snum == y.snum &&
    x.ncum == y.ncum &&
    x.csum == y.csum &&
#endif

    x.str == y.str;
}

#endif // TEST_HXX
