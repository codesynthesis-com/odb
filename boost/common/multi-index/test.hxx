// file      : boost/common/multi-index/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include <odb/core.hxx>

namespace mi = boost::multi_index;

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

typedef
mi::multi_index_container<
  int,
  mi::indexed_by<mi::sequenced<> >
> int_lst;

typedef
mi::multi_index_container<
  int,
  mi::indexed_by<mi::random_access<> >
> int_vec;

typedef
mi::multi_index_container<
  int,
  mi::indexed_by<mi::ordered_unique<mi::identity<int> > >
> int_set;

typedef
mi::multi_index_container<
  int,
  mi::indexed_by<
    mi::sequenced<>,
    mi::ordered_unique<mi::identity<int> >
  >
> int_lst_set;

typedef
mi::multi_index_container<
  comp,
  mi::indexed_by<
    mi::ordered_unique<mi::member<comp, std::string, &comp::str> >,
    mi::random_access<>
  >
> comp_set_vec;

typedef
mi::multi_index_container<
  comp,
  mi::indexed_by<
    mi::ordered_unique<mi::member<comp, int, &comp::num> >,
    mi::ordered_unique<mi::member<comp, std::string, &comp::str> >
  >
> comp_set_set;

#pragma db object
struct object
{
  object () {}
  object (const std::string& id): id (id) {}

  #pragma db id
  std::string id;

  int_lst il;
  int_lst iv;
  int_set is;

  int_lst_set ils;
  comp_set_vec csv;
  comp_set_set css;
};

inline bool
operator== (const object& x, const object& y)
{
  return
    x.id == y.id &&

    x.il == y.il &&
    x.iv == y.iv &&
    x.is == y.is &&

    x.ils == y.ils &&
    x.csv == y.csv &&
    x.css == y.css;
}

#endif // TEST_HXX
