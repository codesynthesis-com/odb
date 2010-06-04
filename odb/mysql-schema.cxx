// file      : odb/mysql-schema.cxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

#include <odb/mysql-schema.hxx>

namespace
{
  struct data_member: traversal::data_member, context
  {
    data_member (context& c)
        : context (c), first_ (true)
    {
    }

    virtual void
    traverse (type& m)
    {
      if (first_)
        first_ = false;
      else
        os << "," << endl;

      os << "  `" << column_name (m) << "` " << db_type (m) <<
        " NOT NULL";

      if (m.count ("id"))
        os << " PRIMARY KEY";
    }

  private:
    bool first_;
  };

  struct class_create: traversal::class_, context
  {
    class_create (context& c)
        : context (c)
    {
    }

    virtual void
    traverse (type& c)
    {
      if (c.file () != unit.file ())
        return;

      if (!c.count ("object"))
        return;

      os << "CREATE TABLE `" << table_name (c) << "` (" << endl;

      {
        data_member m (*this);
        traversal::names n (m);
        names (c, n);
      }

      os << ")";

      string const& engine (options.mysql_engine ());

      if (engine != "default")
        os << endl
           << "  ENGINE=" << engine;

      os << ";" << endl
         << endl;
    }
  };

  struct class_drop: traversal::class_, context
  {
    class_drop (context& c)
        : context (c)
    {
    }

    virtual void
    traverse (type& c)
    {
      if (c.file () != unit.file ())
        return;

      if (!c.count ("object"))
        return;

      os << "DROP TABLE IF EXISTS `" << table_name (c) << "`;" << endl;
    }
  };
}

static char const file_header[] =
 "/* This file was generated by CodeSynthesis ODB object persistence\n"
 " * compiler for C++.\n"
 " */\n\n";

void
generate_mysql_schema (context& ctx)
{
  ctx.os << file_header;

  // Drop.
  //
  {
    traversal::unit unit;
    traversal::defines unit_defines;
    traversal::namespace_ ns;
    class_drop c (ctx);

    unit >> unit_defines >> ns;
    unit_defines >> c;

    traversal::defines ns_defines;

    ns >> ns_defines >> ns;
    ns_defines >> c;
    unit.dispatch (ctx.unit);
  }

  ctx.os << endl;

  // Create.
  //
  {
    traversal::unit unit;
    traversal::defines unit_defines;
    traversal::namespace_ ns;
    class_create c (ctx);

    unit >> unit_defines >> ns;
    unit_defines >> c;

    traversal::defines ns_defines;

    ns >> ns_defines >> ns;
    ns_defines >> c;
    unit.dispatch (ctx.unit);
  }
}
