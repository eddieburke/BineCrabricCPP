@echo off
setlocal enabledelayedexpansion
set "SCRIPT_DIR=%~dp0"

where pwsh.exe >nul 2>nul
if %errorlevel%==0 (
    set "PS_EXE=pwsh.exe"
) else (
    set "PS_EXE=powershell.exe"
)

if "%~1"=="" (
    %PS_EXE% -NoLogo -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-omega.ps1" -Gui
) else (
    %PS_EXE% -NoLogo -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-omega.ps1" %*
)
exit /b %errorlevel%
