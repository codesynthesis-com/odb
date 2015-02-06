// file      : boost/common/unordered/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <string>

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

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

inline std::size_t
hash_value (const comp& x)
{
  std::size_t seed = 0;
  boost::hash_combine (seed, x.num);
  boost::hash_combine (seed, x.str);
  return seed;
}

using boost::unordered_set;
using boost::unordered_multiset;

typedef unordered_set<int> num_set;
typedef unordered_set<std::string> str_set;
typedef unordered_multiset<comp> comp_multiset;

using boost::unordered_map;
using boost::unordered_multimap;

typedef unordered_map<int, std::string> num_str_map;
typedef unordered_map<std::string, int> str_num_map;
typedef unordered_map<int, comp> num_comp_map;
typedef unordered_multimap<comp, std::string> comp_str_multimap;

#pragma db object
struct object
{
  object ()
  {
  }

  object (const std::string& id)
      : id (id)
  {
  }


  #pragma db id
  std::string id;

  // set
  //
  num_set ns;
  str_set ss;
  comp_multiset cms;

  // map
  //
  num_str_map nsm;
  str_num_map snm;
  num_comp_map ncm;
  comp_str_multimap csmm;
};

inline bool
operator== (const object& x, const object& y)
{
  return
    x.id == y.id &&

    x.ns == y.ns &&
    x.ss == y.ss &&
    x.cms == y.cms &&

    x.nsm == y.nsm &&
    x.snm == y.snm &&
    x.ncm == y.ncm &&
    x.csmm == y.csmm;
}

#endif // TEST_HXX
