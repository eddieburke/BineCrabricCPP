# package-mods.ps1  -  Build runtime mod zips from native/mods/<mod_id>/ sources.
#
# Each mod directory must contain mod.json.  Everything inside is zipped into
# <BuildDir>/mods/<mod_id>.zip and deployed to %APPDATA%\.minecraft\mods\.
#
# Usage (run from native/):
#   .\package-mods.ps1
#   .\package-mods.ps1 -BuildDir build-debug
#   .\package-mods.ps1 -ModId camera          # package only one mod
#   .\package-mods.ps1 -NoDeploy              # build zips but skip deploy
#
param(
    [string]$BuildDir = "build-omega",
    [string]$ModId    = "",
    [switch]$NoDeploy
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$ModsSource  = Join-Path $ScriptDir "mods"
$ModsOut     = Join-Path $ScriptDir (Join-Path $BuildDir "mods")
$DeployDir   = Join-Path $env:APPDATA ".minecraft\mods"

New-Item -ItemType Directory -Force -Path $ModsOut | Out-Null

if (-not $NoDeploy) {
    New-Item -ItemType Directory -Force -Path $DeployDir | Out-Null
}

$modDirs = Get-ChildItem -LiteralPath $ModsSource -Directory
if ($ModId -ne "") {
    $modDirs = $modDirs | Where-Object { $_.Name -eq $ModId }
    if ($modDirs.Count -eq 0) {
        Write-Error "Mod '$ModId' not found in $ModsSource"
        exit 1
    }
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$packed = 0
$deployed = 0

foreach ($dir in $modDirs) {
    $manifest = Join-Path $dir.FullName "mod.json"
    if (-not (Test-Path -LiteralPath $manifest)) {
        continue
    }

    $id  = $dir.Name
    $zip = Join-Path $ModsOut "$id.zip"

    if (Test-Path -LiteralPath $zip) {
        Remove-Item -LiteralPath $zip -Force
    }

    $files = Get-ChildItem -Recurse -File -LiteralPath $dir.FullName

    $stream  = [System.IO.File]::Open($zip, [System.IO.FileMode]::Create)
    $archive = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create, $false, [System.Text.Encoding]::UTF8)

    try {
        foreach ($file in $files) {
            $entryName   = $file.FullName.Substring($dir.FullName.Length + 1).Replace("\", "/")
            $entry       = $archive.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
            $entryStream = $entry.Open()
            try {
                $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
                $entryStream.Write($bytes, 0, $bytes.Length)
            } finally {
                $entryStream.Close()
            }
        }
    } finally {
        $archive.Dispose()
        $stream.Dispose()
    }

    $size = [math]::Round((Get-Item -LiteralPath $zip).Length / 1KB, 1)
    Write-Host "  Packed $id  ->  $zip  ($size KB)"
    $packed++

    if (-not $NoDeploy) {
        $dest = Join-Path $DeployDir "$id.zip"
        Copy-Item -LiteralPath $zip -Destination $dest -Force
        Write-Host "  Deployed $id  ->  $dest"
        $deployed++
    }
}

if ($packed -eq 0) {
    Write-Host "No mods found to package."
} else {
    Write-Host ""
    Write-Host "Packaged $packed mod(s)$(if (-not $NoDeploy) { ", deployed $deployed" })."
}
