@echo off
REM Script to build the Open Watcom tools
if not exist %OWROOT%\build\%OWOBJDIR% mkdir %OWROOT%\build\%OWOBJDIR%
set OWBUILDER_BOOTX_OUTPUT=%OWROOT%\build\%OWOBJDIR%\bootx.log
if exist %OWBUILDER_BOOTX_OUTPUT% del %OWBUILDER_BOOTX_OUTPUT%
cd %OWROOT%\bld\wmake
if not exist %OWOBJDIR% mkdir %OWOBJDIR%
cd %OWOBJDIR%
if exist %OWROOT%\build\%OWOBJDIR%\wmake.exe del %OWROOT%\build\%OWOBJDIR%\wmake.exe
wmake -f ..\wmake clean >>%OWBUILDER_BOOTX_OUTPUT% 2>&1
wmake -f ..\wmake >>%OWBUILDER_BOOTX_OUTPUT% 2>&1
if errorlevel == 1 goto error_exit
cd %OWROOT%\bld\builder
if not exist %OWOBJDIR% mkdir %OWOBJDIR%
cd %OWOBJDIR%
if exist %OWROOT%\build\%OWOBJDIR%\builder.exe del %OWROOT%\build\%OWOBJDIR%\builder.exe
%OWROOT%\build\%OWOBJDIR%\wmake -f ..\binmake clean >>%OWBUILDER_BOOTX_OUTPUT% 2>&1
%OWROOT%\build\%OWOBJDIR%\wmake -f ..\binmake bootstrap=1 >>%OWBUILDER_BOOTX_OUTPUT% 2>&1
if errorlevel == 1 goto error_exit
if "%1" == "preboot" goto error_exit
cd "%OWROOT%\bld"
builder boot
if errorlevel == 1 goto error_exit
if "%1" == "boot" goto error_exit
if "%1" == "" goto build
builder %1
goto error_exit
:build
builder build
:error_exit
cd %OWROOT%
