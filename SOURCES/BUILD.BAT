@echo off

if exist EXIST.TMP del /q EXIST.TMP

4dos /c echo.>EXIST.TMP
if not exist EXIST.TMP goto no4dos
del /q EXIST.TMP

goto args

:args
  if "%1"=="" goto call_build_btm
  if "%1"=="clean" goto clean
  if "%1"=="apprel" goto target
  if "%1"=="4dos" goto target
  if "%1"=="batcomp" goto target
  if "%1"=="kstack" goto target
  if "%1"=="all" goto target
  if "%1"=="-h" goto help
  if "%1"=="--help" goto help
  echo Unknown argument: %1
  echo Use BUILD --help for usage.
  goto end

:help
  4dos /c build.btm --help
  goto end

:clean
  4dos /c build.btm clean
  goto end

:target
  4dos /c build.btm %1
  goto end

:call_build_btm
  4dos /c build.btm
  goto end

:no4dos
  echo ERROR: 4DOS.COM is not available
  goto end

:end
