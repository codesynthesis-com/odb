@echo off
rem file      : tester.bat
rem author    : Boris Kolpackov <boris@codesynthesis.com>
rem copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem Run an ODB test. The test directory is the current directory.
rem %1     database id, for example, mysql
rem %2     configuration, for example, Debug or x64/Debug
rem topdir variable containing the path to top project directory

rem clear errorlevel
rem setlocal & endlocal

rem echo %1
rem echo %2
rem echo "%topdir%"

if "_%DIFF%_" == "__" set DIFF=fc

if exist test.sql (
  %topdir%\%1-driver test.sql
  if errorlevel 1 goto error
)

rem echo %2\driver.exe --options-file %topdir%\%1.options

if exist test.std (
  %2\driver.exe --options-file %topdir%\%1.options >test.out
  if errorlevel 1 goto error
  %DIFF% test.std test.out
    
  if errorlevel 1 (
    del /f test.out
    goto error
  ) 
  
  del /f test.out  
  goto end
  
) else (

  %2\driver.exe --options-file %topdir%\%1.options
  if errorlevel 1 goto error
)

goto end


:error
exit /b 1
goto end

:end
