# ODB - ORM for C++

ODB is an open-source, cross-platform, and cross-database object-relational
mapping (ORM) system for C++. It allows you to persist C++ objects to a
relational database without having to deal with tables, columns, or SQL and
without manually writing any mapping code. ODB supports MySQL, SQLite,
PostgreSQL, Oracle, and Microsoft SQL Server relational databases. It also
comes with optional profiles for Boost and Qt which allow you to seamlessly
use value types, containers, and smart pointers from these libraries in your
persistent C++ classes.

For further information, including licensing conditions, documentation, and
binary packages, refer to the [ODB project
page](https://codesynthesis.com/products/odb/).

NOTE: the steps described below are more appropriate for the development of
ODB as opposed to consumption. In case you just want to use ODB:

* If you want to use a binary package, see the [ODB download
  page](https://codesynthesis.com/products/odb/download.xhtml).

* If you want to build ODB from source and use the result from a project that
  uses a build system other than `build2`, then see [Installing ODB with
  `build2`](https://codesynthesis.com/products/odb/doc/install-build2.xhtml)
  for the step-by-step instructions.

* If you want to use ODB from a project that uses `build2` as the build
  system, then see the accompanying `PACKAGE-README.md` file.

The development setup for ODB uses multiple build configurations: a single
host configuration for the ODB compiler and a target configuration per
database. For example:

```
git clone .../odb.git
cd odb

bdep init --empty

bdep config create @host ../odb-host --type host cc config.cxx=g++
bdep config create @sqlite ../odb-sqlite cc config.cxx=g++
bdep config create @pgsql ../odb-pgsql cc config.cxx=g++
bdep config create @mysql ../odb-mysql cc config.cxx=g++
bdep config create @oracle ../odb-oracle cc config.cxx=g++
bdep config create @mssql ../odb-mssql cc config.cxx=g++

bdep init @host -d odb
bdep init @sqlite -d libodb -d libodb-sqlite -d odb-tests
bdep init @pgsql -d libodb -d libodb-pgsql -d odb-tests
bdep init @mysql -d libodb -d libodb-mysql -d odb-tests
bdep init @oracle -d libodb -d libodb-oracle -d odb-tests
bdep init @mssql -d libodb -d libodb-mssql -d odb-tests
```

Note that while the target configurations can use any compiler (and you can
create multiple such configurations for different compilers), the host
configuration must use GCC because the ODB compiler is implemented as a GCC
plugin.

You can use system-installed versions of the client libraries and tools
instead of building them from source. For example:

```
bdep config link @pgsql @host
bdep config link @mysql @host

bdep init @sqlite -d libodb -d libodb-sqlite -d odb-tests ?sys:libsqlite3
bdep init @pgsql -d libodb -d libodb-pgsql -d odb-tests ?sys:libpq { @host }+ ?sys:psql/*
bdep init @mysql -d libodb -d libodb-mysql -d odb-tests ?sys:libmysqlclient { @host }+ ?sys:mysql-client/*
```

Note that the client libraries and tools for Oracle and Microsoft SQL Server
are expected to be system-installed.

You can also create a target configuration for testing multi-database
support. For example:

```
bdep config create @multi ../odb-multi cc config.cxx=g++

bdep init @multi -d libodb -d libodb-sqlite -d libodb-pgsql -d libodb-mysql
bdep init @multi -d odb-tests config.odb_tests.database='sqlite pgsql mysql'
```

See `odb-tests/README.md` for instructions on setting up various databases to
run tests.

To generate the documentation in the `.ps` and `.pdf` formats, the `html2ps`
and `ps2pdf14` programs are required (the latter is from `ghostscript`). A
warning is issued in the development mode if these programs are not available.

To test installation of the ODB compiler, create a separate target
configuration (it will automatically resolve any build-time dependencies from
`host`):

```
bdep config create @install ../odb-install --type target cc config.cxx=g++ \
  config.install.root=/tmp/install

bdep init @install -d odb

b install: ../odb-install/odb/
```

Note that if building manually (that is, without `bdep`) and in-source, then
the checked out `odb/odb/version.hxx`, `libodb/odb/version.hxx`, and
`libodb-*/odb/*/version.hxx` files will be overwritten during the build but
these changes should be ignored. To do this automatically, run:

```
git update-index --assume-unchanged odb/odb/version.hxx libodb/odb/version.hxx libodb-*/odb/*/version.hxx
```
