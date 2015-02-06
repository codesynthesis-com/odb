@echo off
rem file      : tester.bat
rem copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
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

rem Globbing returns files in alphabetic order.
rem
if exist test*.sql (
  for %%f in (test*.sql) do (
    call %topdir%\%1-driver.bat %%f
    if errorlevel 1 goto error
  )
)

if exist test-%1.std (
  set "std=test-%1.std"
) else (
  if exist test.std (
    set "std=test.std"
  ) else (
    set "std="
  )
)

if "_%std%" == "__" (

  %dir%\driver.exe --options-file %topdir%\%1.options
  if errorlevel 1 goto error
  
) else (

  %dir%\driver.exe --options-file %topdir%\%1.options >test.out
  if errorlevel 1 goto error
  %DIFF% %std% test.out

  if errorlevel 1 (
    del /f test.out
    goto error
  )

  del /f test.out
  goto end

)

goto end

:error
endlocal
exit /b 1

:end
endlocal
