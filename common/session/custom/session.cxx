// file      : common/session/custom/session.cxx
// copyright : Copyright (c) 2009-2012 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

#include "session.hxx"

static session* current_; // Use TLS in multi-threaded applications.

session::
session ()
{
  assert (current_ == 0);
  current_ = this;
}

session::
~session ()
{
  assert (current_ == this);
  current_ = 0;
}

bool session::
has_current ()
{
  return current_ != 0;
}

session& session::
current ()
{
  assert (current_ != 0);
  return *current_;
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
