// file      : common/include/test1.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST1_HXX
#define TEST1_HXX

// Test include directive parsing.
//
#include"obj1.hxx"

   #   include     \
     <common/include/obj2.hxx>

/*comment*/ # /*comment*/ include /* comment */ "obj3.hxx" // comment

#endif // TEST1_HXX
