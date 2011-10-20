// file      : common/container/test.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>

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
operator< (const comp& x, const comp& y)
{
  return x.num != y.num ? x.num < y.num : x.str < y.str;
}

typedef std::list<std::string> str_list;

typedef std::vector<int> num_vector;
typedef std::vector<std::string> str_vector;

typedef std::set<int> num_set;
typedef std::set<std::string> str_set;

typedef std::map<int, std::string> num_str_map;
typedef std::map<std::string, int> str_num_map;
typedef std::map<int, comp> num_comp_map;

#pragma db value
struct cont_comp1
{
  // This composite value does not have any columns.
  //
  #pragma db id_column("obj_id")
  num_vector sv; // Have the name "conflic" with the one in the object.
};

#pragma db value
struct cont_comp2
{
  cont_comp2 (): num (777), str ("ggg") {}

  int num;

  #pragma db id_column("obj_id")
  str_list sl;

  std::string str;
};

#pragma db object table("obj")
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

  // set
  //
  num_set ns;
  str_set ss;
  std::set<comp> cs;

  // map
  //
  num_str_map nsm;
  str_num_map snm;
  num_comp_map ncm;
  std::map<comp, std::string> csm;

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

    x.ns == y.ns &&
    x.ss == y.ss &&
    x.cs == y.cs &&

    x.nsm == y.nsm &&
    x.snm == y.snm &&
    x.ncm == y.ncm &&
    x.csm == y.csm &&

    x.str == y.str;
}

#endif // TEST_HXX
