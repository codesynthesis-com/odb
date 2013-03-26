// file      : common/changelog/alter-column.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ALTER_COLUMN_HXX
#define ALTER_COLUMN_HXX

#pragma db model version(BVER, CVER, true)

#pragma db object
struct object
{
  #pragma db id auto
  int id;

#if CVER > 1
  #pragma db null
#endif
  int num;
};

#endif // ALTER_COLUMN_HXX
