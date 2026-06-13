# Cleans the omega build directory and lock files
$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$BuildDir = "build-omega"
$LockPath = ".build-omega.lock"

Write-Host "Cleaning Omega build..."

if (Test-Path $BuildDir) {
    Write-Host "Removing $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

if (Test-Path $LockPath) {
    Write-Host "Removing $LockPath"
    Remove-Item -Force $LockPath
}

Write-Host "Clean complete."
