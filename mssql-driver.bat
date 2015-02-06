@echo off
rem file      : mssql-driver.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem mssql-driver.bat sql-file
rem
rem Run the mssql client on the SQL file specified. Adjust the
rem options below to match your SQL Server setup.
rem

setlocal

set "options=%MSSQL_OPTIONS%"

rem User.
rem
set "options=%options% -U odb_test"

rem Password.
rem
set "options=%options% -P odb_test"

rem Database name.
rem
set "options=%options% -d odb_test"

rem SQL Server instance address.
rem
rem set "options=%options% -S host\instance"
rem set "options=%options% -S tcp:host,port"

rem Standard options.
rem
set "options=%options% -x -r -b"

set "mssql=%MSSQL_CLIENT%"

if "_%mssql%_" == "__" set "mssql=sqlcmd"

if "_%1_" == "__" (
  echo no sql file specified
  goto usage
)

%mssql% %options% -i %1

if errorlevel 1 goto error
goto end

:usage
echo.
echo usage: mssql-driver.bat sql-file
echo.

:error
endlocal
exit /b 1

:end
endlocal
