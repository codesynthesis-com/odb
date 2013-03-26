// file      : common/changelog/drop-column.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef DROP_COLUMN_HXX
#define DROP_COLUMN_HXX

#pragma db model version(BVER, CVER, true)

#pragma db object
struct object
{
  #pragma db id auto
  int id;

#if CVER == 1
  int num;
#endif
};

#endif // DROP_COLUMN_HXX
