// file      : evolution/combined/model.hxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef MODEL_VERSION
#  error model.hxx included directly
#endif

#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include <common/config.hxx> // DATABASE_XXX

#pragma db model version(1, MODEL_VERSION)

#define MODEL_NAMESPACE_IMPL(V) v##V
#define MODEL_NAMESPACE(V) MODEL_NAMESPACE_IMPL(V)

namespace MODEL_NAMESPACE(MODEL_VERSION)
{
  #pragma db object
  struct object1
  {
    object1 (unsigned long id = 0): id_ (id) {}

    #pragma db id
    unsigned long id_;
  };

  #pragma db object
  struct object
  {
    #pragma db id
    std::string id_;

    //
    // Pre pass 1.
    //

    // Drop unique index.
    //
#if MODEL_VERSION == 2
    #pragma db unique
#endif
    unsigned long dui;

    // Alter table drop foreign key. Not supported by SQLite.
    //
#ifndef DATABASE_SQLITE
#if MODEL_VERSION == 2
    object1* dfk;
#else
    #pragma db null
    unsigned long dfk;
#endif
#endif

    // Add table.
    //
#if MODEL_VERSION == 3
    std::vector<std::string> at;
#endif

    // Add column.
    //
#if MODEL_VERSION == 3
    #pragma db default(999)
    unsigned long ac1;

    odb::nullable<unsigned long> ac2;

    // Initially added as NULL, converted to NOT NULL. Not supported by SQLite.
    //
#ifndef DATABASE_SQLITE
    #pragma db not_null
    odb::nullable<unsigned long> ac3;
#endif
#endif

    // Alter column NULL. Not supported by SQLite.
    //
#ifndef DATABASE_SQLITE
#if MODEL_VERSION == 2
    unsigned long acn;
#else
    odb::nullable<unsigned long> acn;
#endif
#endif

    //
    // Pre pass 2.
    //

    // Add non-unique indexes.
    //
#if MODEL_VERSION == 3
    #pragma db index
#endif
    unsigned long anui;

    //
    // Post pass 1.
    //

    // Drop non-unique indexes.
    //
#if MODEL_VERSION == 2
    #pragma db index
#endif
    unsigned long dnui;

    //
    // Post pass 2.
    //

    // Drop table.
    //
#if MODEL_VERSION == 2
    std::vector<unsigned long> dt;
#endif

    // Drop column. Logical drop (set NULL) in SQLite.
    //
#if MODEL_VERSION == 2
    unsigned long dc;
#endif

    // Alter column NOT NULL. Not supported by SQLite.
    //
#ifndef DATABASE_SQLITE
#if MODEL_VERSION == 3
    #pragma db not_null
#endif
    odb::nullable<unsigned long> acnn;
#endif

    // Alter table add foreign key. Not supported by SQLite.
    //
#ifndef DATABASE_SQLITE
#if MODEL_VERSION == 2
    #pragma db null
    unsigned long afk;
#else
    object1* afk;
#endif
#endif

    // Add unique index.
    //
#if MODEL_VERSION == 3
    #pragma db unique
#endif
    unsigned long aui;

  public:
#ifndef DATABASE_SQLITE
#if MODEL_VERSION == 2

    object (std::string id = ""): id_ (id), dfk (0) {}
    ~object () {delete dfk;}
#else
    object (std::string id = ""): id_ (id), afk (0) {}
    ~object () {delete afk;}
#endif
#else
    object (std::string id = ""): id_ (id) {}
#endif
  };
}

#undef MODEL_NAMESPACE
#undef MODEL_NAMESPACE_IMPL
