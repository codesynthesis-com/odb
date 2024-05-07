# odb-examples - ODB compiler usage examples

This package contains examples of using `odb`, object-relational mapping (ORM)
compiler for C++. The following list gives an overview of the available
examples. See the `README` file accompanying each example for more
information. See `odb-tests/README.md` for instructions on setting up various
databases to run these examples.

Note also that most of the examples use the --table-prefix ODB compiler
option to assign a unique prefix to tables created by each example. This
is done to allow examples to run against the same  database without
causing any schema conflicts. You don't have to use this option in your
own applications.

## hello

A "Hello World" example that shows how to use ODB to perform basic database
operations.

## query

Shows how to use the ODB Query Language to search the database for persistent
objects matching certain criteria.

## composite

Shows how to declare and use composite value types.

## container

Shows how to use containers as data members in persistent objects.

## relationship

Shows how to declare and use unidirectional to-one and to-many relationships.

## inverse

Shows how to declare and use bidirectional one-to-one, one-to-many, and
many-to-many relationships.

## inheritance/reuse

Shows how to use reuse inheritance with ODB.

## inheritance/polymorphism

Shows how to use polymorphism inheritance with ODB.

## section

Shows how to use object sections to implement lazy-loading and change-updating
of a subset of data members in a persistent class.

## view

Shows how to define and use object, table, mixed, and native views.

## prepared

Shows how to use prepared queries.

## optimistic

Shows how to use optimistic concurrency in ODB.

## pimpl

Shows how to use virtual data members to implement a persistent class that
employs the pimpl C++ idiom.

## c++11

Shows how to use ODB with C++11.

## access

Shows various approaches used by ODB to access data members that cannot be
accessed directly.

## boost

Shows how to persist objects that use Boost smart pointers, containers, and
value types with the help of the Boost profile library (libodb-boost).

## qt

Shows how to persist objects that use Qt smart pointers, containers, and value
types with the help of the Qt profile library (libodb-qt).

## schema/embedded

Shows how to generate and use a database schema that is embedded into the
application.

## schema/custom

Shows how to map persistent C++ classes to a custom database schema.

## mapping

Shows how to customize the mapping between C++ value types and database types.
