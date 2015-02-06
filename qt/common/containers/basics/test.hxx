// file      : qt/common/containers/basics/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QLinkedList>
#include <QtCore/QSet>
#include <QtCore/QMap>
#include <QtCore/QMultiMap>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>

#include <odb/core.hxx>

#pragma db value
struct comp
{
  comp () {}
  comp (int n, const QString& s) : num (n), str (s) {}

  #pragma db column("number")
  int num;
  QString str;
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

typedef QVector<int> num_vector;
typedef QVector<QString> str_vector;
typedef QVector<comp> comp_vector;

typedef QList<int> num_list;
typedef QList<QString> str_list;
typedef QList<comp> comp_list;

typedef QLinkedList<int> num_linked_list;
typedef QLinkedList<QString> str_linked_list;
typedef QLinkedList<comp> comp_linked_list;

typedef QSet<int> num_set;
typedef QSet<QString> str_set;

typedef QMap<int, QString> num_str_map;
typedef QMap<QString, int> str_num_map;
typedef QMap<int, comp> num_comp_map;
typedef QMap<comp, QString> comp_str_map;

typedef QMultiMap<int, QString> num_str_multimap;
typedef QMultiMap<QString, int> str_num_multimap;
typedef QMultiMap<int, comp> num_comp_multimap;

typedef QHash<int, QString> num_str_hash;
typedef QHash<QString, int> str_num_hash;
typedef QHash<QString, comp> str_comp_hash;

typedef QMultiHash<int, QString> num_str_multihash;
typedef QMultiHash<QString, int> str_num_multihash;
typedef QMultiHash<int, comp> num_comp_multihash;

#pragma db value
struct cont_comp1
{
  // This composite value does not have any columns.
  //
  num_vector sv; // Have the name "conflict" with the one in the object.
};

#pragma db value
struct cont_comp2
{
  cont_comp2 (): num (777), str ("ggg") {}

  int num;
  str_list sl;
  QString str;
};

#pragma db object
struct object
{
  object (): nv (comp1_.sv), sl (comp2_.sl) {}
  object (const QString& id) : id_ (id), nv (comp1_.sv), sl (comp2_.sl) {}

  #pragma db id
  QString id_;

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
  comp_vector cv;

  #pragma db unordered
  num_vector uv;

  // list
  //
  #pragma db transient
  str_list& sl;

  num_list nl;
  comp_list cl;

  // linked list
  //
  str_linked_list sll;
  num_linked_list nll;
  comp_linked_list cll;

  // set
  //
  num_set ns;
  str_set ss;

  // map
  //
  num_str_map nsm;
  str_num_map snm;
  num_comp_map ncm;
  comp_str_map csm;

  // multimap
  //
  num_str_multimap nsmm;
  str_num_multimap snmm;
  num_comp_multimap ncmm;

  // hash
  //
  num_str_hash nsh;
  str_num_hash snh;
  str_comp_hash sch;

  // multihash
  //
  num_str_multihash nsmh;
  str_num_multihash snmh;
  num_comp_multihash ncmh;

  QString str;
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
    x.nl == y.nl &&
    x.cl == y.cl &&

    x.nll == y.nll &&
    x.sll == y.sll &&
    x.cll == y.cll &&

    x.ns == y.ns &&
    x.ss == y.ss &&

    x.nsm == y.nsm &&
    x.snm == y.snm &&
    x.ncm == y.ncm &&
    x.csm == y.csm &&

    x.nsmm.uniqueKeys () == y.nsmm.uniqueKeys () &&
    x.snmm.uniqueKeys () == y.snmm.uniqueKeys () &&
    x.ncmm.uniqueKeys () == y.ncmm.uniqueKeys () &&

    x.nsh == y.nsh &&
    x.snh == y.snh &&
    x.sch == y.sch &&

    x.nsmh.uniqueKeys () == y.nsmh.uniqueKeys () &&
    x.snmh.uniqueKeys () == y.snmh.uniqueKeys () &&
    x.ncmh.uniqueKeys () == y.ncmh.uniqueKeys () &&

    x.str == y.str;
}

#endif // TEST_HXX
