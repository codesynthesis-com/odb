@echo off
rem file      : common/test.bat
rem author    : Boris Kolpackov <boris@codesynthesis.com>
rem copyright : Copyright (c) 2009-2010 Code Synthesis Tools CC
rem license   : GNU GPL v2; see accompanying LICENSE file

setlocal

set "tests=__path__(dirs) __path__(thread_dirs)"
set "confs=__path__(configurations)"
set "topdir=__path__(topdir)\.."

goto start

:run_test
  cd %1

  if exist %2\driver.exe (
    echo %1\%2
    call %topdir%\tester.bat tracer %2
    if errorlevel 1 (
      set "failed=%failed% %1\%2"
    )
  )

  cd ..
goto :eof

:start

for %%t in (%tests%) do (
  for %%c in (%confs%) do (
    call :run_test %%t %%c
  )
)

if not "_%failed%_" == "__" goto error

echo ALL TESTS PASSED
goto end

:error
for %%t in (%failed%) do echo FAILED: %%t
exit /b 1
goto end

:end
endlocal
