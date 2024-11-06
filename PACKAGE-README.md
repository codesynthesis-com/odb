# ODB - ORM for C++

ODB is an open-source, cross-platform, and cross-database object-relational
mapping (ORM) system for C++. It allows you to persist C++ classes to a
relational database without having to deal with tables, columns, or SQL and
without manually writing any mapping code. ODB supports the MySQL, SQLite,
PostgreSQL, Oracle, and Microsoft SQL Server relational databases. It also
comes with optional profiles for Boost and Qt which allow you to seamlessly
use value types, containers, and smart pointers from these libraries in your
persistent C++ classes.

For further information, refer to the [ODB project
page](https://codesynthesis.com/products/odb/).

## Usage

ODB consists of several packages with the main ones being `odb` (the ODB
compiler), `libodb` (the common runtime library), and `libodb-<database>` (the
database runtime libraries). There are also `libodb-boost` and `libodb-qt`
(profile libraries) as well as `odb-tests` and `odb-examples`.

When specifying dependencies on the ODB packages in your project, the `odb`
package should be a build-time dependency. You will always have a dependency
on `libodb` plus one or more `libodb-<database>`, depending on which
database(s) you wish to target. To be able to persist types from either Boost
or Qt you would also add the corresponding profile library.

So, putting it all together, your project's `manifest` would normally have the
following fragment if, for example, you want to target SQLite:

```
depends: * odb ^2.5.0
depends: libodb ^2.5.0
depends: libodb-sqlite ^2.5.0
```

Or the following fragment if using PostgreSQL as well as the Boost profile:

```
depends: * odb ^2.5.0
depends: libodb ^2.5.0
depends: libodb-pgsql ^2.5.0
depends: libodb-boost ^2.5.0
```

Then your `buildfile` would have something along these lines if using
SQLite:

```
import! [metadata] odb = odb%exe{odb}

import libs  = libodb%lib{odb}
import libs += libodb-sqlite%lib{odb-sqlite}
```

Or along these lines if using PostgreSQL and the Boost profile:

```
import! [metadata] odb = odb%exe{odb}

import libs  = libodb%lib{odb}
import libs += libodb-sqlite%lib{odb-sqlite}
import libs += libodb-boost%lib{odb-boost}
```

Note that the `odb` executable provides `build2` metadata.

The invocation of the ODB compiler (in order to generate the database support
code from your headers) can be implemented using ad hoc recipes or rules. See
the `odb-examples` package for the complete examples.
