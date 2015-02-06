@echo off
rem file      : evolution/test.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

setlocal

set "tests=__path__(dirs) __path__(thread_dirs)"
set "confs=__path__(configurations)"
set "plats=__path__(platforms)"
set "curdir=%CD%"
set "topdir=%curdir%\.."
set "failed="

if "_%1_" == "__" (
  echo no database specified
  goto usage
)

goto start

rem
rem %1 - test directory
rem %2 - configuration
rem %3 - platform
rem %4 - database
rem
:run_test
  cd %1

  if "_%3_" == "_Win32_" (
    set "dir=%2"
  ) else (
    set "dir=%3\%2"
  )

  if exist %dir%\driver.exe (
    echo %1\%3\%2
    call %topdir%\evolution\tester.bat %4 %2 %3
    if errorlevel 1 (
      set "failed=%failed% %1\%3\%2"
    )
  )

  cd %curdir%
goto :eof

:start

for %%t in (%tests%) do (
  for %%c in (%confs%) do (
    for %%p in (%plats%) do (
      call :run_test %%t %%c %%p %1
    )
  )
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
