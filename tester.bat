@echo off
rem file      : tester.bat
rem copyright : Copyright (c) 2009-2018 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

rem
rem Run an ODB example. The example directory is the current directory.
rem
rem %1     database
rem %2     configuration, for example, Debug or Release
rem %3     platform, for example Win32 or x64
rem topdir variable containing the path to top project directory
rem

setlocal

if "_%3_" == "_Win32_" (
  set "dir=%2"
) else (
  set "dir=%3\%2"
)

rem Globbing returns files in alphabetic order.
rem
if exist *.sql (
  for %%f in (*.sql) do (
    call %topdir%\%1-driver.bat %%f
    if errorlevel 1 goto error
  )
)

%dir%\driver.exe --options-file %topdir%\%1.options
if errorlevel 1 goto error

goto end

:error
endlocal
exit /b 1

:end
endlocal
