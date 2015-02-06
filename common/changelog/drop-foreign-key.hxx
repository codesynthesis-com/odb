// file      : common/changelog/drop-foreign-key.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef DROP_FOREIGN_KEY_HXX
#define DROP_FOREIGN_KEY_HXX

#pragma db model version(BVER, CVER, open)

struct object1;

#pragma db object
struct object
{
  #pragma db id auto
  int id;

#if CVER == 1
  object1* o1;
#endif
};

#pragma db object
struct object1
{
  #pragma db id
  int id;
};

#endif // DROP_FOREIGN_KEY_HXX
