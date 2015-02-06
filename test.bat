@echo off
rem file      : test.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem test.bat database
rem
rem Run tests built with VC++.
rem

setlocal

set "failed="
set "curdir=%CD%"

if "_%1_" == "__" (
  echo no database specified
  goto usage
)


goto start

rem
rem %1 - directory
rem %2 - database
rem
:run_test
  echo.
  echo testing %1
  echo.
  cd %1
  call test.bat %2
  if errorlevel 1 set "failed=%failed% %1"
  cd %curdir%
goto :eof

:start

for %%d in (common evolution %1 boost\common boost\%1 qt\common qt\%1) do (
  call :run_test %%d %1
)

if not "_%failed%_" == "__" goto error

echo.
echo ALL TESTS PASSED
echo.
goto end

:usage
echo.
echo usage: test.bat database
echo.

:error
if not "_%failed%_" == "__" (
  echo.
  for %%t in (%failed%) do echo FAILED: %%t
  echo.
)
endlocal
exit /b 1

:end
endlocal
