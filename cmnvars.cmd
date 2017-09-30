@echo off
REM *****************************************************************
REM CMNVARS.CMD - common environment variables
REM *****************************************************************
REM NOTE: All scripts to set the environment must call this script at
REM       the end.

REM Set the version numbers
set OWBLDVER=20
set OWBLDVERSTR=2.0

REM Set up default path information variable
if not "%OWDEFPATH%" == "" goto defpath_set
set OWDEFPATH=%PATH%;
set OWDEFINCLUDE=%INCLUDE%
set OWDEFWATCOM=%WATCOM%
set OWDEFBEGINLIBPATH=%BEGINLIBPATH%
:defpath_set

REM Subdirectory to be used for building OW build tools
if "%OWOBJDIR%" == "" set OWOBJDIR=binbuild

REM Subdirectory to be used for build binaries
set OWBINDIR=%OWROOT%\build\%OWOBJDIR%

REM Subdirectory containing OW sources
set OWSRCDIR=%OWROOT%\bld

REM Subdirectory containing documentation sources
set OWDOCSDIR=%OWROOT%\docs

REM Set environment variables
set PATH=%OWBINDIR%;%OWROOT%\build;%OWDEFPATH%
set INCLUDE=%OWDEFINCLUDE%
set WATCOM=%OWDEFWATCOM%
set BEGINLIBPATH=%OWDEFBEGINLIBPATH%

REM Set Watcom tool chain version to WATCOMVER variable
set WATCOMVER=0
if not '%OWTOOLS%' == 'WATCOM' goto no_watcom
echo set WATCOMVER=__WATCOMC__>watcom.gc
wcc386 -p watcom.gc >watcom.bat
watcom.bat
del watcom.*
:no_watcom

REM OS specifics

REM Ensure COMSPEC points to CMD.EXE
set COMSPEC=CMD.EXE

echo Open Watcom compiler build environment

