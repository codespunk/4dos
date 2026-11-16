@echo off

if exist EXIST.TMP del EXIST.TMP

4dos /c echo.>EXIST.TMP
if not exist EXIST.TMP goto no4dos
del EXIST.TMP

:args
   if "%1"=="" goto checktools
   if "%1"=="clean" goto clean
   if "%1"=="-h" goto help
   if "%1"=="--help" goto help
   echo Unknown argument: %1
   echo Use BUILD --help for usage.
   goto end

:help
   echo 4DOS Build Script
   echo.
   echo Usage: BUILD [clean]
   echo.
   echo Builds 4DOS using MKDOS.BTM.
   echo By default, performs an incremental build without deleting files.
   echo.
   echo Commands:
   echo   clean       Delete files in SOURCES\DOS and exit
   echo   -h, --help  Display this help and exit
   goto end

:clean
   4dos /c del /y /e DOS\*.BAK DOS\*.COM DOS\*.EXE DOS\*.MAP DOS\*.OBJ 4DOSTART.BAK 4DOSTART.LIB CRITERRS.ASM INITERRS.ASM PARSERRS.ASM SERVERRS.ASM
   goto end

:checktools
   if "%MSVC%"=="" goto badMSVC
   if "%MASM%"=="" goto badMASM

   if not exist %MSVC%\NUL goto badMSVC
   if not exist %MASM%\NUL goto badMASM

   if not exist %MSVC%\BIN\CL.EXE goto noCL
   if not exist %MASM%\BIN\ML.EXE goto noML
   goto build

:badMSVC
   echo ERROR: %%MSVC%% is undefined or does not point to a valid directory
   goto end

:badMASM
   echo ERROR: %%MASM%% is undefined or does not point to a valid directory
   goto end

:noCL
   echo ERROR: CL.EXE not found in "%MSVC%\BIN"
   goto end

:noML
   echo ERROR: ML.EXE not found in "%MASM%\BIN"
   goto end

:build
   if not exist INTVER.ASM 4dos /c build.btm
   4dos /e:2048 /c mkdos.btm
   goto end

:no4dos
   echo ERROR: 4DOS.COM is not available.
   goto end

:end
