// file      : common/statement/processing/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test internal statement processing machinery.
//

#include <string>
#include <cassert>
#include <iostream>

#include <odb/statement.hxx>

using namespace std;

static bool
insert (const char* stmt,
        const char* expected,
        const void* const* bind,
        size_t bind_size)
{
  string r;
  odb::statement::process_insert (
    stmt, bind, bind_size, sizeof (void*), '$', r);
  return r == expected;
}

static bool
update (const char* stmt,
        const char* expected,
        const void* const* bind,
        size_t bind_size)
{
  string r;
  odb::statement::process_update (
    stmt, bind, bind_size, sizeof (void*), '$', r);
  return r == expected;
}

static bool
select (const char* stmt,
        const char* expected,
        const void* const* bind,
        size_t bind_size)
{
  string r;
  odb::statement::process_select (
    stmt, bind, bind_size, sizeof (void*), '[', ']', true, r);
  return r == expected;
}

int
main (int, char* argv[])
{
  //
  // INSERT
  //

  // Fast path.
  //
  {
    void* b[] = {argv, argv};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b])\n"
                    "VALUES\n"
                    "(DEFAULT,\n$1)",
                    "INSERT INTO [foo] ([a], [b]) VALUES (DEFAULT, $1)",
                    b, 2));
  }

  // Empty via statement.
  //
  /* LIBODB_DEBUG_STATEMENT_PROCESSING
  {
    assert (insert ("INSERT INTO [foo]\n"
                    "DEFAULT VALUES",
                    "INSERT INTO [foo] DEFAULT VALUES",
                    0, 0));
  }
  */

  // Empty via bind.
  //
  {
    void* b[] = {0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a])\n"
                    "VALUES\n"
                    "($1)",
                    "INSERT INTO [foo] DEFAULT VALUES",
                    b, 1));
  }

  // Empty with OUTPUT.
  //
  {
    void* b[] = {0, 0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b])\n"
                    "OUTPUT INSERTED.[id]\n"
                    "VALUES\n"
                    "($1,\n$2)",
                    "INSERT INTO [foo] OUTPUT INSERTED.[id] DEFAULT VALUES",
                    b, 2));
  }

  // Empty with RETURNING.
  //
  {
    void* b[] = {0, 0, 0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "($1,\n$1,\n$2)\n"
                    "RETURNING [id]",
                    "INSERT INTO [foo] DEFAULT VALUES RETURNING [id]",
                    b, 3));
  }

  // Empty via bind, but not values.
  //
  {
    void* b[] = {0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b])\n"
                    "VALUES\n"
                    "(1,\n$1)",
                    "INSERT INTO [foo] ([a]) VALUES (1)",
                    b, 1));
  }

  // Empty via bind, but not values.
  //
  {
    void* b[] = {0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "(1,\n$1,\nDEFAULT)",
                    "INSERT INTO [foo] ([a], [c]) VALUES (1, DEFAULT)",
                    b, 1));
  }

  // First not present.
  //
  {
    void* b[] = {0, argv, argv};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "($1,\n$2,\n$3)",
                    "INSERT INTO [foo] ([b], [c]) VALUES ($2, $3)",
                    b, 3));
  }

  // Last not present.
  //
  {
    void* b[] = {argv, argv, 0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "($1,\n$2,\n$3)",
                    "INSERT INTO [foo] ([a], [b]) VALUES ($1, $2)",
                    b, 3));
  }

  // Middle not present.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "($1,\n$2,\n$3)",
                    "INSERT INTO [foo] ([a], [c]) VALUES ($1, $3)",
                    b, 3));
  }

  // Multiple not present.
  //
  {
    void* b[] = {0, argv, 0, argv, 0};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c],\n[d],\n[e])\n"
                    "VALUES\n"
                    "($1,\n$2,\n$3,\n$4,\n$5)",
                    "INSERT INTO [foo] ([b], [d]) VALUES ($2, $4)",
                    b, 5));
  }

  // Not present and OUTPUT.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "OUTPUT INSERTED.[id]\n"
                    "VALUES\n"
                    "($1,\n$2,\n$3)",
                    "INSERT INTO [foo] ([a], [c]) OUTPUT INSERTED.[id] "
                    "VALUES ($1, $3)",
                    b, 3));
  }

  // Not present and RETURNING.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "($1,\n$2,\n$3)\n"
                    "RETURNING [id]",
                    "INSERT INTO [foo] ([a], [c]) VALUES ($1, $3) "
                    "RETURNING [id]",
                    b, 3));
  }

  // Value expressions.
  //
  {
    void* b[] = {argv, argv, argv};
    assert (insert ("INSERT INTO [foo]\n"
                    "([a],\n[b],\n[c])\n"
                    "VALUES\n"
                    "($1,\nCAST($2, TEXT),\n$3)",
                    "INSERT INTO [foo] ([a], [b], [c]) "
                    "VALUES ($1, CAST($2, TEXT), $3)",
                    b, 3));
  }

  //
  // UPDATE
  //

  // Fast path.
  //
  {
    void* b[] = {argv, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "ver=ver+1,\n[a]=$1\n"
                    "WHERE [id]=$2",
                    "UPDATE [foo] SET ver=ver+1, [a]=$1 WHERE [id]=$2",
                    b, 2));
  }

  // Empty via bind.
  //
  {
    void* b[] = {0, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1\n"
                    "WHERE [id]=$2",
                    "",
                    b, 2));
  }

  // Empty via bind, but not values.
  //
  {
    void* b[] = {0, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "ver=ver+1,\n[a]=$1\n"
                    "WHERE [id]=$2",
                    "UPDATE [foo] SET ver=ver+1 WHERE [id]=$2",
                    b, 2));
  }

  // First not present.
  //
  {
    void* b[] = {0, argv, argv, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=$2,\n"
                    "[c]=$3\n"
                    "WHERE [id]=$4",
                    "UPDATE [foo] SET [b]=$2, [c]=$3 WHERE [id]=$4",
                    b, 4));
  }

  // Last not present.
  //
  {
    void* b[] = {argv, argv, 0, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=$2,\n"
                    "[c]=$3\n"
                    "WHERE [id]=$4",
                    "UPDATE [foo] SET [a]=$1, [b]=$2 WHERE [id]=$4",
                    b, 4));
  }

  // Middle not present.
  //
  {
    void* b[] = {argv, 0, argv, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=$2,\n"
                    "[c]=$3\n"
                    "WHERE [id]=$4",
                    "UPDATE [foo] SET [a]=$1, [c]=$3 WHERE [id]=$4",
                    b, 4));
  }

  // Multiple not present.
  //
  {
    void* b[] = {0, argv, 0, argv, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=$2,\n"
                    "[c]=$3,\n"
                    "[d]=$4\n"
                    "WHERE [id]=$5",
                    "UPDATE [foo] SET [b]=$2, [d]=$4 WHERE [id]=$5",
                    b, 5));
  }

  // Not present and OUTPUT.
  //
  {
    void* b[] = {argv, 0, argv, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=$2,\n"
                    "[c]=$3\n"
                    "OUTPUT INSERTED.[ver] "
                    "WHERE [id]=$4",
                    "UPDATE [foo] SET [a]=$1, [c]=$3 OUTPUT INSERTED.[ver] "
                    "WHERE [id]=$4",
                    b, 4));
  }

  // Value expressions.
  //
  {
    void* b[] = {argv, argv, argv, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=CAST($2, TEXT),\n"
                    "[c]=$3\n"
                    "WHERE [id]=$4",
                    "UPDATE [foo] SET [a]=$1, [b]=CAST($2, TEXT), [c]=$3 "
                    "WHERE [id]=$4",
                    b, 4));
  }

  // No OUTPUT/WHERE clause.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (update ("UPDATE [foo]\n"
                    "SET\n"
                    "[a]=$1,\n"
                    "[b]=$2,\n"
                    "[c]=$3",
                    "UPDATE [foo] SET [a]=$1, [c]=$3",
                    b, 4));
  }

  //
  // SELECT
  //

  // Empty.
  //
  {
    void* b[] = {0, 0, 0};
    assert (select ("SELECT\n"
                    "[a].[x],\n"
                    "[t].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "",
                    b, 3));
  }

  // Fast path.
  //
  {
    void* b[] = {argv, argv};
    assert (select ("SELECT\n"
                    "[s].[t].[x],\n"
                    "[a].[y]\n"
                    "FROM [s].[t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[s].[t].[id]\n"
                    "WHERE [s].[t].[id]=$1",
                    "SELECT [s].[t].[x], [a].[y] FROM [s].[t] "
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[s].[t].[id] "
                    "WHERE [s].[t].[id]=$1",
                    b, 2));
  }

  // First not present.
  //
  {
    void* b[] = {0, argv, argv};
    assert (select ("SELECT\n"
                    "[a].[x],\n"
                    "[t].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[y], [t].[z] FROM [t] WHERE [t].[id]=$1",
                    b, 3));
  }

  // Last not present.
  //
  {
    void* b[] = {argv, argv, 0};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[t].[y],\n"
                    "[a].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[x], [t].[y] FROM [t] WHERE [t].[id]=$1",
                    b, 3));
  }

  // Middle not present.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[a].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[x], [t].[z] FROM [t] WHERE [t].[id]=$1",
                    b, 3));
  }

  // Multiple not present.
  //
  {
    void* b[] = {0, argv, 0, argv};
    assert (select ("SELECT\n"
                    "[a1].[w],\n"
                    "[t].[x],\n"
                    "[a2].[y],\n"
                    "[a3].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a1] ON [a1].[id]=[t].[id]\n"
                    "LEFT JOIN [t2] AS [a2] ON [a2].[id]=[t].[id]\n"
                    "LEFT JOIN [t3] AS [a3] ON [a3].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[x], [a3].[z] FROM [t] "
                    "LEFT JOIN [t3] AS [a3] ON [a3].[id]=[t].[id] "
                    "WHERE [t].[id]=$1",
                    b, 4));
  }

  // Column expression.
  //
  {
    void* b[] = {argv, argv, 0};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "CAST([a].[y], TEXT),\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[x], CAST([a].[y], TEXT) FROM [t] "
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id] "
                    "WHERE [t].[id]=$1",
                    b, 3));
  }

  // No WHERE.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[t].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]",
                    "SELECT [t].[x], [t].[z] FROM [t]",
                    b, 3));
  }

  // JOIN without WHERE.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[a].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]",
                    "SELECT [t].[x], [t].[z] FROM [t]",
                    b, 3));
  }

  // JOIN presence because of WHERE.
  //
  {
    void* b[] = {argv, 0, argv};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[a].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1 AND [a].[id]=$2",
                    "SELECT [t].[x], [t].[z] FROM [t] "
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id] "
                    "WHERE [t].[id]=$1 AND [a].[id]=$2",
                    b, 3));
  }


  // JOIN presence because of dependent JOIN.
  //
  {
    void* b[] = {argv, argv, argv};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[a_b].[y],\n"
                    "[t].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [d] AS [a_d] ON [a_d].[id]=[t].[id]\n"
                    "LEFT JOIN [b] AS [a_b] ON [a_b].[id]=[a_d].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[x], [a_b].[y], [t].[z] FROM [t] "
                    "LEFT JOIN [d] AS [a_d] ON [a_d].[id]=[t].[id] "
                    "LEFT JOIN [b] AS [a_b] ON [a_b].[id]=[a_d].[id] "
                    "WHERE [t].[id]=$1",
                    b, 3));
  }

  // JOIN without alias and with schema.
  //
  {
    void* b[] = {argv, argv, argv};
    assert (select ("SELECT\n"
                    "[t].[x],\n"
                    "[s].[t1].[y],\n"
                    "[t2].[z]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [s].[t1] ON [s].[t1].[id]=[t].[id]\n"
                    "LEFT JOIN [t2] ON [t2].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [t].[x], [s].[t1].[y], [t2].[z] FROM [t] "
                    "LEFT JOIN [s].[t1] ON [s].[t1].[id]=[t].[id] "
                    "LEFT JOIN [t2] ON [t2].[id]=[t].[id] "
                    "WHERE [t].[id]=$1",
                    b, 3));
  }

  // JOIN alias top-level qualifer test.
  //
  {
    void* b[] = {argv, 0};
    assert (select ("SELECT\n"
                    "[s].[a].[x],\n"
                    "[a].[y]\n"
                    "FROM [s].[a]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[s].[a].[id]\n"
                    "WHERE [s].[a].[id]=$1",
                    "SELECT [s].[a].[x] FROM [s].[a] WHERE [s].[a].[id]=$1",
                    b, 2));
  }

  // JOIN alias bottom-level qualifer test (FROM case).
  //
  {
    void* b[] = {argv, 0};
    assert (select ("SELECT\n"
                    "[a].[t].[x],\n"
                    "[a].[y]\n"
                    "FROM [a].[t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[a].[t].[id]\n"
                    "WHERE [a].[t].[id]=$1",
                    "SELECT [a].[t].[x] FROM [a].[t] WHERE [a].[t].[id]=$1",
                    b, 2));
  }

  // JOIN alias bottom-level qualifer test (LEFT JOIN case).
  //
  {
    void* b[] = {0, argv};
    assert (select ("SELECT\n"
                    "[a].[y],\n"
                    "[a].[t2].[x]\n"
                    "FROM [t]\n"
                    "LEFT JOIN [t1] AS [a] ON [a].[id]=[t].[id]\n"
                    "LEFT JOIN [a].[t2] ON [a].[t2].[id]=[t].[id]\n"
                    "WHERE [t].[id]=$1",
                    "SELECT [a].[t2].[x] FROM [t] "
                    "LEFT JOIN [a].[t2] ON [a].[t2].[id]=[t].[id] "
                    "WHERE [t].[id]=$1",
                    b, 2));
  }
}
