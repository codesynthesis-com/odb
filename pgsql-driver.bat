@echo off
rem file      : pgsql-driver.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem pgsql-driver.bat sql-file
rem
rem Run the pgsql client on the SQL file specified. Adjust the
rem options below to match your PostgreSQL setup.
rem

setlocal

set "options=%PGSQL_OPTIONS%"

rem This user must be able to login without specifying a password.
rem
set "options=%options% --username=odb_test"

set "options=%options% --dbname=odb_test"
rem set "options=%options% --host="
rem set "options=%options% --port="
set "options=%options% --quiet"
set "PGOPTIONS=--client-min-messages=warning"

set "pgsql=%PGSQL_CLIENT%"

if "_%pgsql%_" == "__" set "pgsql=psql"

if "_%1_" == "__" (
  echo no sql file specified
  goto usage
)

%pgsql% %options% < %1

if errorlevel 1 goto error
goto end

:usage
echo.
echo usage: pgsql-driver.bat sql-file
echo.

:error
endlocal
exit /b 1

:end
endlocal
