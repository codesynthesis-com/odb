# libodb-sqlite - SQLite ODB runtime library

ODB is an open-source, cross-platform, and cross-database object-relational
mapping (ORM) system for C++. It allows you to persist C++ classes to a
relational database without having to deal with tables, columns, or SQL and
without manually writing any mapping code.

For further information, including licensing conditions, documentation, and
binary packages, refer to the [ODB project
page](https://codesynthesis.com/products/odb/).

This package contains the SQLite ODB runtime library. Applications that
include code generated for the SQLite database will need to link this library.

This runtime library in turn depends on `libsqlite3`, SQLite client library.

If you plan to access an SQLite database from multiple threads, then you will
need SQLite version `3.5.0` or later built with the unlock notify feature
(`SQLITE_ENABLE_UNLOCK_NOTIFY`) enabled.

If you plant to use SQLite incremental BLOB/TEXT I/O support, then you will
need SQLite version `3.4.0` or later built with the column metadata functions
(`SQLITE_ENABLE_COLUMN_METADATA`) enabled.
