# libodb-mysql - MySQL ODB runtime library

ODB is an open-source, cross-platform, and cross-database object-relational
mapping (ORM) system for C++. It allows you to persist C++ classes to a
relational database without having to deal with tables, columns, or SQL and
without manually writing any mapping code.

For further information, including licensing conditions, documentation, and
binary packages, refer to the [ODB project
page](https://codesynthesis.com/products/odb/).

This package contains the MySQL ODB runtime library. Applications that include
code generated for the MySQL database will need to link this library.

This runtime library in turn depends on `libmysqlclient`, MySQL client
library. Note that the MariaDB client library can be used instead of MySQL.
