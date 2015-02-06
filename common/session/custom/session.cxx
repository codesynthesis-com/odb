// file      : common/session/custom/session.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <cassert>

#include "session.hxx"

session* session::current;

session::
session ()
    : tran_ (0)
{
  assert (current == 0);
  current = this;
}

session::
~session ()
{
  // Unregister from transaction.
  //
  if (tran_ != 0)
    tran_->callback_unregister (this);

  assert (current == this);
  current = 0;
}

void session::
flush (odb::database& db)
{
  bool flushed (false);

  for (type_map::iterator i (map_.begin ()), e (map_.end ()); i != e; ++i)
  {
    bool r (i->second->flush (db));
    flushed = flushed || r;
  }

  // If we flushed anything, then register the post-commit/rollback callback.
  //
  if (flushed)
  {
    tran_ = &odb::transaction::current ();
    tran_->callback_register (
      &mark, this, odb::transaction::event_all, 0, &tran_);
  }
}

void session::
mark (unsigned short event, void* key, unsigned long long)
{
  session& s (*static_cast<session*> (key));
  for (type_map::iterator i (s.map_.begin ()), e (s.map_.end ()); i != e; ++i)
    i->second->mark (event);
}
