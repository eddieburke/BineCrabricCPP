@echo off
rem Double-clickable launcher for build-omega.ps1.
rem Windows' default "Open with PowerShell" / file association often runs Windows
rem PowerShell 5.1 (powershell.exe) even when PowerShell 7 (pwsh.exe) is installed.
rem This forces pwsh.exe when present, falling back to powershell.exe otherwise.
rem
rem Usage:
rem   build.bat              -> runs a Release build, then pops a folder picker
rem   build.bat -BuildType Debug   -> forwards any args straight to build-omega.ps1
setlocal
set "SCRIPT_DIR=%~dp0"

where pwsh.exe >nul 2>nul
if %errorlevel%==0 (
    set "PS_EXE=pwsh.exe"
) else (
    set "PS_EXE=powershell.exe"
)

if "%~1"=="" (
    %PS_EXE% -NoLogo -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-omega.ps1"
) else (
    %PS_EXE% -NoLogo -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-omega.ps1" %*
)
exit /b %errorlevel%
