// file      : common/changelog/add-column.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ADD_COLUMN_HXX
#define ADD_COLUMN_HXX

#pragma db model version(BVER, CVER, open)

#pragma db object
struct object
{
  #pragma db id auto
  int id;

#if CVER > 1
  int num;
#endif
};

#endif // ADD_COLUMN_HXX
