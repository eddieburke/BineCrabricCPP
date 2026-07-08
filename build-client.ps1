# Build the Minecraft client (minecraft_native.exe).
#
# Usage:
#   .\build-client.ps1
#   .\build-client.ps1 -Clean
#   .\build-client.ps1 -Run
#   .\build-client.ps1 -Run -- --username Player
#
# Forwards -Clean, -Jobs, -BuildType, -BuildDir, -Lto, -NoLto, -NoNativeCpu,
# and -RunTests to build-omega.ps1.

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
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$GameArgs
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$buildArgs = @{
    BuildDir           = $BuildDir
    BuildType          = $BuildType
    Jobs               = $Jobs
    Target             = "Client"
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

$exe = Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_native.exe")
if (-not (Test-Path -LiteralPath $exe)) {
    Write-Error "Client executable not found: $exe"
    exit 1
}

Write-Host "Launching $exe ..."
if ($GameArgs -and $GameArgs.Count -gt 0) {
    & $exe @GameArgs
} else {
    & $exe
}
exit $LASTEXITCODE
