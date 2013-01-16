// file      : common/session/custom/session.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

#include "session.hxx"

session* session::current;

session::
session ()
{
  assert (current == 0);
  current = this;
}

session::
~session ()
{
  assert (current == this);
  current = 0;
}

void session::
flush (odb::database& db)
{
  for (type_map::iterator i (map_.begin ()), e (map_.end ()); i != e; ++i)
    i->second->flush (db);
}

void session::
mark ()
{
  for (type_map::iterator i (map_.begin ()), e (map_.end ()); i != e; ++i)
    i->second->mark ();
}
