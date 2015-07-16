@echo off
@setlocal

set config=Release

rem prevent an error only present on HP computers
set Platform=
set platformcode=

:permissions
rem check admin permissions
net session >nul 2>&1
if not errorlevel 1 goto environment
echo Run this script as administrator.
goto done 

:environment
rem set to current directory
setlocal enableextensions
cd /d "%~dp0"

rem enable Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

:submodules
echo Updating Git submodules...
git submodule update --init --recursive
if errorlevel 1 goto error
echo Done.
goto cinder

:cinder
echo.
echo Compiling cinder...
msbuild "..\cinder_master\vc2013\cinder.sln" /m /p:Configuration=%config%
if errorlevel 1 goto error
echo Done.
goto samples

:samples
echo.
echo Compiling samples...
if exist build.log del /F build.log
for /R %%f in (*.sln) do msbuild %%f /m /p:Configuration=%config% /fl /flp:logfile=build.log;verbosity=minimal;append=true
if errorlevel 1 goto error
echo Done.
goto done

:error
echo An error occurred. See README.md for more information.

:done
echo.
pause