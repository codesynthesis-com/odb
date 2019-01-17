// file      : common/changelog/add-index.hxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ADD_INDEX_HXX
#define ADD_INDEX_HXX

#pragma db model version(BVER, CVER, open)

#pragma db object
struct object
{
  #pragma db id auto
  int id;

  int x;

#if CVER > 1
  int y;
  #pragma db index ("xy_i") unique member(x) member(y, "DESC")
#endif

};

#endif // ADD_INDEX_HXX
