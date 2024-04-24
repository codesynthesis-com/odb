// file      : pgsql/bulk/driver.cxx
// license   : GNU GPL v2; see accompanying LICENSE file

// Test transaction savepoints.
//

#include <libpq-fe.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/select.h>

// Note: hack (also, some platforms, like Mac OS, provide it).
//
#include <arpa/inet.h>
#ifndef htonll
#  define htonll(x) ((((long long)htonl(x)) << 32) + htonl((x) >> 32))
#endif

#undef NDEBUG
#include <assert.h>

static const size_t columns = 3;

struct data
{
  long long id;
  long long idata;
  const char* sdata;
};

static char* values[columns];
static int lengths[columns];
static int formats[columns] = {1, 1, 1};

static const unsigned int types[columns] = {
  20, // int8
  20, // int8
  25  // text
};

static void
init (const struct data* d)
{
  values[0] = (char*)&d->id;
  lengths[0] = sizeof (d->id);

  values[1] = (char*)&d->idata;
  lengths[1] = sizeof (d->idata);

  values[2] = (char*)d->sdata;
  lengths[2] = strlen (d->sdata);
}

static void
execute (PGconn* conn, const struct data* ds, size_t n)
{
  int sock = PQsocket (conn);
  assert (sock != -1);

  if (PQsetnonblocking (conn, 1) == -1 ||
      PQenterPipelineMode (conn) == 0)
    assert (false);

  // True if we've written and read everything, respectively.
  //
  bool wdone = false;
  bool rdone = false;

  size_t wn = 0;
  size_t rn = 0;

  while (!rdone)
  {
    fd_set wds;
    if (!wdone)
    {
      FD_ZERO (&wds);
      FD_SET (sock, &wds);
    }

    fd_set rds;
    FD_ZERO (&rds);
    FD_SET (sock, &rds);

    if (select (sock + 1, &rds, wdone ? NULL : &wds, NULL, NULL) == -1)
    {
      if (errno == EINTR)
        continue;

      assert (false);
    }

    // Try to minimize the chance of blocking the server by first processing
    // the result and then sending more queries.
    //
    if (FD_ISSET (sock, &rds))
    {
      if (PQconsumeInput (conn) == 0)
        assert (false);

      while (wn > rn && PQisBusy (conn) == 0)
      {
        //fprintf (stderr, "PQgetResult %zu\n", rn);

        PGresult* res = PQgetResult (conn);
        assert (res != NULL);
        ExecStatusType stat = PQresultStatus (res);

        if (stat == PGRES_PIPELINE_SYNC)
        {
          assert (wdone && rn == n);
          PQclear (res);
          rdone = true;
          break;
        }

        if (stat == PGRES_FATAL_ERROR)
        {
          const char* s = PQresultErrorField (res, PG_DIAG_SQLSTATE);

          if (strcmp (s, "23505") == 0)
            fprintf (stderr, "duplicate id at %zu\n", rn);
        }

        PQclear (res);
        assert (rn != n);
        ++rn;

        // We get a NULL result after each query result.
        //
        {
          PGresult* end = PQgetResult (conn);
          assert (end == NULL);
        }
      }
    }

    if (!wdone && FD_ISSET (sock, &wds))
    {
      // Send queries until we get blocked (write-biased). This feels like
      // a better overall strategy to keep the server busy compared to
      // sending one query at a time and then re-checking if there is
      // anything to read because the results of INSERT/UPDATE/DELETE are
      // presumably small and quite a few of them can get buffered before
      // the server gets blocked.
      //
      for (;;)
      {
        if (wn < n)
        {
          //fprintf (stderr, "PQsendQueryPrepared %zu\n", wn);

          init (ds + wn);

          if (PQsendQueryPrepared (conn,
                                   "persist_object",
                                   (int)(columns),
                                   values,
                                   lengths,
                                   formats,
                                   1) == 0)
            assert (false);

          if (++wn == n)
          {
            if (PQpipelineSync (conn) == 0)
              assert (false);

            ++wn;
          }
        }

        // PQflush() result:
        //
        //  0  --  success (queue is now empty)
        //  1  --  blocked
        // -1  --  error
        //
        int r = PQflush (conn);
        assert (r != -1);

        if (r == 0)
        {
          if (wn < n)
          {
            // If we continue here, then we are write-biased. And if we
            // break, then we are read-biased.
            //
#if 0
            break;
#else
            continue;
#endif
          }

          wdone = true;
        }

        break; // Blocked or done.
      }
    }
  }

  if (PQexitPipelineMode (conn) == 0 ||
      PQsetnonblocking (conn, 0) == -1)
    assert (false);
}

static void
test (PGconn* conn)
{
  const size_t batch = 500;
  struct data ds[batch];

  for (size_t i = 0; i != batch; ++i)
  {
    ds[i].id = htonll (i == batch / 2 ? i - 1 : i); // Cause duplicate PK.
    ds[i].idata = htonll (i);
    ds[i].sdata = "abc";
  }

  // Prepare the statement.
  //
  {
    PGresult* res = PQprepare (
      conn,
      "persist_object",
      "INSERT INTO \"pgsql_bulk_object\" "
      "(\"id\", "
      "\"idata\", "
      "\"sdata\") "
      "VALUES "
      "($1, $2, $3)",
      (int)(columns),
      types);
    assert (PQresultStatus (res) == PGRES_COMMAND_OK);
    PQclear (res);
  }

  // Begin transaction.
  //
  {
    PGresult* res = PQexec (conn, "begin");
    assert (PQresultStatus (res) == PGRES_COMMAND_OK);
    PQclear (res);
  }

  execute (conn, ds, batch);

  // Commit transaction.
  //
  {
    PGresult* res = PQexec (conn, "commit");
    assert (PQresultStatus (res) == PGRES_COMMAND_OK);
    PQclear (res);
  }
}

#include <vector>
#include <memory>   // std::unique_ptr
#include <cstring>
#include <iostream>

#include <odb/pgsql/database.hxx>
#include <odb/pgsql/transaction.hxx>

#include <libcommon/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
namespace pgsql = odb::pgsql;
using namespace pgsql;

int
main (int argc, char* argv[])
{
  bool fail_already_persistent (false);

  for (int i (1); i != argc; ++i)
  {
    if (strcmp (argv[i], "--fail-already-persistent") == 0)
    {
      fail_already_persistent = true;

      for (; i != argc - 1; ++i)
        argv[i] = argv[i + 1];

      --argc;

      break;
    }
  }

  try
  {
    unique_ptr<database> db (create_specific_database<database> (argc, argv));

    connection_ptr cn (db->connection ());

    if (false)
    {
      PGconn* conn (cn->handle ());
      test (conn);
    }

    {
      const unsigned long n (500);

      vector<object> os;

      for (unsigned long i (0); i != n; ++i)
      {
        os.push_back (object {i, i, string (i, 'x')});

        if (fail_already_persistent && i == n / 2)
          os.push_back (object {i, i, to_string (i)});
      }

      {
        transaction t (cn->begin ());
        db->persist (os.begin (), os.end ());
        t.commit ();
      }

      {
        transaction t (cn->begin ());
        db->find<object> (2);
        t.commit ();
      }

      for (unsigned long i (0); i != n; ++i)
      {
        //assert (os[i].id == i + 1);
        os[i].idata++;
      }

      {
        transaction t (cn->begin ());
        db->update (os.begin (), os.end ());
        t.commit ();
      }

      {
        transaction t (cn->begin ());
        db->erase (os.begin (), os.end ());
        t.commit ();
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
