@echo off
rem file      : mysql-driver.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem mysql-driver.bat sql-file
rem
rem Run the mysql client on the SQL file specified. Adjust the
rem options below to match your MySQL setup.
rem

setlocal

set "options=%MYSQL_OPTIONS%"
set "options=%options% --user=odb_test"
set "options=%options% --database=odb_test"
rem set "options=%options% --password="
rem set "options=%options% --host="
rem set "options=%options% --port="
rem set "options=%options% --socket="

set "mysql=%MYSQL_CLIENT%"

if "_%mysql%_" == "__" set "mysql=mysql"

if "_%1_" == "__" (
  echo no sql file specified
  goto usage
)

%mysql% %options% < %1

if errorlevel 1 goto error
goto end

:usage
echo.
echo usage: mysql-driver.bat sql-file
echo.

:error
endlocal
exit /b 1

:end
endlocal
