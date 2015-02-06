@echo off
rem file      : oracle-driver.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem oracle-driver.bat sql-file
rem
rem Run the oracle client on the SQL file specified. Adjust the
rem options below to match your Oracle setup.
rem

setlocal

set "options=%ORACLE_OPTIONS% -L -S"
set "options=%options% odb_test/odb_test"
rem set "options=%options% odb_test/odb_test@//192.168.0.5:1521/xe"

set "oracle=%ORACLE_CLIENT%"

if "_%oracle%_" == "__" set "oracle=sqlplus"

if "_%1_" == "__" (
  echo no sql file specified
  goto usage
)

%oracle% %options% @%1

if errorlevel 1 goto error
goto end

:usage
echo.
echo usage: oracle-driver.bat sql-file
echo.

:error
endlocal
exit /b 1

:end
endlocal
