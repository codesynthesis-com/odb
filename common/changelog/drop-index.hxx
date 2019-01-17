// file      : common/changelog/drop-index.hxx
// copyright : Copyright (c) 2009-2019 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef DROP_INDEX_HXX
#define DROP_INDEX_HXX

#pragma db model version(BVER, CVER, open)

#pragma db object
struct object
{
  #pragma db id auto
  int id;

#if CVER == 1
  #pragma db index
#endif
  int num;
};

#endif // DROP_INDEX_HXX
