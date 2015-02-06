// file      : odb/pgsql/pgsql-fwd.hxx
// copyright : Copyright (c) 2005-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#ifndef ODB_PGSQL_PGSQL_FWD_HXX
#define ODB_PGSQL_PGSQL_FWD_HXX

#include <odb/pre.hxx>

// Forward declaration for some of the types defined in libpq-fe.h. This
// allows us to avoid having to include libpq-fe.h in public headers.
//
typedef unsigned int Oid;

typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

#include <odb/post.hxx>

#endif // ODB_PGSQL_PGSQL_FWD_HXX
