// file      : odb/generate.hxx
// copyright : Copyright (c) 2009-2013 Code Synthesis Tools CC
// license   : GNU GPL v3; see accompanying LICENSE file

#ifndef ODB_GENERATE_HXX
#define ODB_GENERATE_HXX

namespace include
{
  bool
  generate (bool header);
}

namespace header
{
  void
  generate ();
}

namespace inline_
{
  void
  generate ();
}

namespace source
{
  void
  generate ();
}

#endif // ODB_GENERATE_HXX
