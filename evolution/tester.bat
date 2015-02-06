@echo off
rem file      : evolution/tester.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem Run an evolution test. The test directory is the current directory.
rem
rem %1     database
rem %2     configuration, for example, Debug or Release
rem %3     platform, for example Win32 or x64
rem topdir variable containing the path to top project directory
rem

setlocal

set "PATH=%topdir%\libcommon\bin64;%topdir%\libcommon\bin;%PATH%"

if "_%3_" == "_Win32_" (
  set "dir=%2"
) else (
  set "dir=%3\%2"
)

if exist test*.sql (
  rem Standalone schema.
  rem

  rem Drop everything.
  rem
  call %topdir%\%1-driver.bat test3.sql
  if errorlevel 1 goto error

  call %topdir%\%1-driver.bat test2.sql
  if errorlevel 1 goto error

  call %topdir%\%1-driver.bat test1.sql
  if errorlevel 1 goto error

  rem Base schema.
  rem
  call %topdir%\%1-driver.bat test3-002-pre.sql
  if errorlevel 1 goto error

  call %topdir%\%1-driver.bat test3-002-post.sql
  if errorlevel 1 goto error

  %dir%\driver.exe --options-file %topdir%\%1.options 1
  if errorlevel 1 goto error

  rem Migration.
  rem
  call %topdir%\%1-driver.bat test3-003-pre.sql
  if errorlevel 1 goto error

  %dir%\driver.exe --options-file %topdir%\%1.options 2
  if errorlevel 1 goto error

  call %topdir%\%1-driver.bat test3-003-post.sql
  if errorlevel 1 goto error

  rem Current schema.
  rem
  %dir%\driver.exe --options-file %topdir%\%1.options 3
  if errorlevel 1 goto error

) else (

  rem Embedded schema. Just run the driver.
  rem
  %dir%\driver.exe --options-file %topdir%\%1.options 1
  if errorlevel 1 goto error

  %dir%\driver.exe --options-file %topdir%\%1.options 2
  if errorlevel 1 goto error

  %dir%\driver.exe --options-file %topdir%\%1.options 3
  if errorlevel 1 goto error
)

goto end

:error
endlocal
exit /b 1

:end
endlocal
