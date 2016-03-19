@echo off
setlocal EnableDelayedExpansion

set __ProjectDir=%~dp0
set __ThisScriptShort=%0
set __ThisScriptFull="%~f0"

set __BuildArch=
set __DotNetHostBinDir=

:Arg_Loop
if "%1" == "" goto ArgsDone

if /i "%1" == "/?"    goto Usage
if /i "%1" == "-?"    goto Usage
if /i "%1" == "/h"    goto Usage
if /i "%1" == "-h"    goto Usage
if /i "%1" == "/help" goto Usage
if /i "%1" == "-help" goto Usage

if /i "%1" == "x64"                 (set __BuildArch=%1&shift&goto Arg_Loop)
if /i "%1" == "x86"                 (set __BuildArch=%1&shift&goto Arg_Loop)
if /i "%1" == "arm"                 (set __BuildArch=%1&shift&goto Arg_Loop)
if /i "%1" == "arm64"               (set __BuildArch=%1&shift&goto Arg_Loop)
if /i "%1" == "/hostbindir"         (set __DotNetHostBinDir=%2&shift&shift&goto Arg_Loop)

echo Invalid command line argument: %1
goto Usage

:ArgsDone

if [%__BuildArch%]==[] (goto Usage)
if [%__DotNetHostBinDir%]==[] (goto Usage)

:: Initialize the MSBuild Tools
call %__ProjectDir%\init-tools.cmd

:: Restore dependencies mainly to obtain runtime.json
pushd %__ProjectDir%\deps
%__ProjectDir%\Tools\dotnetcli\bin\dotnet.exe restore --source "https://dotnet.myget.org/F/dotnet-core" --packages %__ProjectDir%\packages
popd

:: Clean up existing nupkgs
rmdir /s /q %__ProjectDir%\bin

:: Package the assets using Tools
%__ProjectDir%\Tools\corerun %__ProjectDir%\Tools\MSBuild.exe %__ProjectDir%\projects\Microsoft.NETCore.DotNetHostPolicy.builds /p:Platform=%__BuildArch% /p:DotNetHostBinDir=%__DotNetHostBinDir% /p:TargetsWindows=true
%__ProjectDir%\Tools\corerun %__ProjectDir%\Tools\MSBuild.exe %__ProjectDir%\projects\Microsoft.NETCore.DotNetHostResolver.builds /p:Platform=%__BuildArch% /p:DotNetHostBinDir=%__DotNetHostBinDir% /p:TargetsWindows=true
%__ProjectDir%\Tools\corerun %__ProjectDir%\Tools\MSBuild.exe %__ProjectDir%\projects\Microsoft.NETCore.DotNetHost.builds /p:Platform=%__BuildArch% /p:DotNetHostBinDir=%__DotNetHostBinDir% /p:TargetsWindows=true

exit /b 0

:Usage
echo.
echo Package the dotnet host artifacts
echo.
echo Usage:
echo     %__ThisScriptShort% [x64/x86/arm]  /hostbindir path-to-binaries
echo.
echo./? -? /h -h /help -help: view this message.
