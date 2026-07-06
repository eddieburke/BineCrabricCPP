# Build the dedicated Minecraft server (minecraft_server.exe).
#
# Usage:
#   .\build-server.ps1
#   .\build-server.ps1 -Clean
#   .\build-server.ps1 -Run
#   .\build-server.ps1 -Run -NoGui
#
# Without -NoGui, Win32 dedicated server GUI opens (same as Java without nogui).
# Forwards build flags to build-omega.ps1.

param(
    [string]$BuildDir = "build-omega",
    [ValidateSet("Release", "RelWithDebInfo", "Debug")]
    [string]$BuildType = "Release",
    [int]$Jobs = 0,
    [switch]$Clean,
    [switch]$Lto,
    [switch]$NoLto,
    [switch]$NoNativeCpu,
    [switch]$RunTests,
    [switch]$Run,
    [switch]$NoGui,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ServerArgs
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$buildArgs = @{
    BuildDir  = $BuildDir
    BuildType = $BuildType
    Jobs      = $Jobs
    Target    = "Server"
}
if ($Clean) { $buildArgs.Clean = $true }
if ($Lto) { $buildArgs.Lto = $true }
if ($NoLto) { $buildArgs.NoLto = $true }
if ($NoNativeCpu) { $buildArgs.NoNativeCpu = $true }
if ($RunTests) { $buildArgs.RunTests = $true }

& (Join-Path $ScriptDir "build-omega.ps1") @buildArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if (-not $Run) {
    exit 0
}

$exe = Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_server.exe")
if (-not (Test-Path -LiteralPath $exe)) {
    Write-Error "Server executable not found: $exe"
    exit 1
}

$launchArgs = @()
if ($NoGui) {
    $launchArgs += "nogui"
}
if ($ServerArgs -and $ServerArgs.Count -gt 0) {
    $launchArgs += $ServerArgs
}

Write-Host "Launching $exe ..."
if ($launchArgs.Count -gt 0) {
    & $exe @launchArgs
} else {
    & $exe
}
exit $LASTEXITCODE
