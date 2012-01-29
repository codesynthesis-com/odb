@echo off
rem file      : tester.bat
rem copyright : Copyright (c) 2009-2011 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem Run an ODB test. The test directory is the current directory.
rem
rem %1     database
rem %2     configuration, for example, Debug or Release
rem %3     platform, for example Win32 or x64
rem topdir variable containing the path to top project directory
rem

setlocal

set "PATH=%topdir%\libcommon\bin64;%topdir%\libcommon\bin;%PATH%"

if "_%DIFF%_" == "__" set DIFF=fc

if "_%3_" == "_Win32_" (
  set "dir=%2"
) else (
  set "dir=%3\%2"
)

if exist test.sql (
  call %topdir%\%1-driver.bat test.sql
  if errorlevel 1 goto error
)

if exist test.std (
  %dir%\driver.exe --options-file %topdir%\%1.options >test.out
  if errorlevel 1 goto error
  %DIFF% test.std test.out

  if errorlevel 1 (
    del /f test.out
    goto error
  )

  del /f test.out
  goto end

) else (

  %dir%\driver.exe --options-file %topdir%\%1.options
  if errorlevel 1 goto error
)

goto end

:error
endlocal
exit /b 1

:end
endlocal

