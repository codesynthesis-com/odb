# odb-tests - tests for ODB compiler

This package contains tests for `odb`, object-relational mapping (ORM)
compiler for C++.

## Setting up PostgreSQL for running tests

1. Install the PostgreSQL server. On Linux this is normally done using
   distribution packages.

2. In `/etc/postgresql/N/main/pg_hba.conf` add the following line after the
   `local all postgres ...` line:

   ```
   local   odb_test        odb_test                                trust
   ```

   If you want to be able to run tests on a different host and access
   PostgreSQL via TCP/IP, also add the following line (adjusting the IP
   network to match your setup -- never specify a public IP network here since
   the access is unauthenticated):

   ```
   host    odb_test        odb_test        192.168.0.0/24          trust
   ```

   You will also need to edit `/etc/postgresql/N/main/postgresql.conf` and
   change `listen_address` to `*`. You may also need to open port `5432` in
   your firewall.

   Restart the PostgreSQL server.

3. Add the `odb_test` user and the `odb_test` database.

   First login:

   ```
   sudo -u postgres psql
   ```

   Then execute the following statements:

   ```
   CREATE USER odb_test;
   CREATE DATABASE odb_test;
   GRANT ALL PRIVILEGES ON DATABASE odb_test TO odb_test;
   \c odb_test
   GRANT ALL PRIVILEGES ON SCHEMA public TO odb_test;
   ```

## Setting up MySQL for running tests

1. Install the MySQL server. On Linux this is normally done using distribution
   packages.

2. Setup remote access (optional).

   If you want to be able to run tests on a different host and access MySQL
   via TCP/IP (never do this on a public server since the access is
   unauthenticated), then edit `/etc/mysql/.../mysqld.cnf`, the `[mysqld]`
   section, and change `bind-address` to `0.0.0.0`. You may also need to open
   port `3306` in your firewall.

   Restart the MySQL server.

3. Add the `odb_test` user and the `odb_test` database.

   First login:

   ```
   sudo mysql
   ```

   Then execute the following statements:

   ```
   CREATE USER odb_test@'%';
   CREATE USER odb_test@'localhost';
   CREATE DATABASE odb_test;
   GRANT ALL PRIVILEGES ON odb_test.* to odb_test@'%';
   FLUSH PRIVILEGES;
   ```
