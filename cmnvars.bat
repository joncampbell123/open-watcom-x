@echo off
REM *****************************************************************
REM CMNVARS.BAT - common environment variables
REM *****************************************************************
REM NOTE: All batch files to set the environment must call this batch
REM       file at the end.

REM Set the version numbers
set OWBLDVER=20
set OWBLDVERSTR=2.0

REM Set up default path information variables
if not "%OWDEFPATH%" == "" goto defpath_set
set OWDEFPATH=%PATH%;
set OWDEFINCLUDE=%INCLUDE%
set OWDEFWATCOM=%WATCOM%
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

REM Set Watcom tool chain version to WATCOMVER variable
set WATCOMVER=0
if not '%OWTOOLS%' == 'WATCOM' goto no_watcom
echo set WATCOMVER=__WATCOMC__>watcom.gc
wcc386 -p watcom.gc >watcom.bat
call watcom.bat
del watcom.*
:no_watcom

REM OS specifics

REM setup right COMSPEC for non-standard COMSPEC setting on NT based systems
if not '%OS%' == 'Windows_NT' goto no_windows_nt
if '%NTDOS%' == '1' goto no_windows_nt
set COMSPEC=%WINDIR%\system32\cmd.exe
set COPYCMD=/y
:no_windows_nt

echo Open Watcom build environment
