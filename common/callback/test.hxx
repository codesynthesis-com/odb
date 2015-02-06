// file      : common/callback/test.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef TEST_HXX
#define TEST_HXX

#include <odb/core.hxx>
#include <odb/callback.hxx>

#pragma db object callback(db_callback)
struct object
{
  object (unsigned long id, unsigned long d)
      : id_ (id), data (d), pobj (0), robj (0), ref (0)
  {
  }

  object ()
      : id_ (0), pobj (0), robj (0)
  {
  }

  #pragma db id
  unsigned long id_;

  unsigned long data;

  object* pobj;

  // Test custom recursive loading.
  //
  #pragma db transient
  object* robj;
  unsigned long ref; // Unless 0, reference to another object.

  void
  db_callback (odb::callback_event, odb::database&);

  void
  db_callback (odb::callback_event, odb::database&) const;
};

#endif // TEST_HXX
