param(
    [string]$RunDirectory = "",
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

function Resolve-DefaultRunDirectory {
    if ($RunDirectory -ne "") {
        return $RunDirectory
    }
    if ($env:APPDATA -and $env:APPDATA -ne "") {
        return (Join-Path $env:APPDATA ".minecraft")
    }
    if ($env:USERPROFILE -and $env:USERPROFILE -ne "") {
        return (Join-Path $env:USERPROFILE ".minecraft")
    }
    return (Join-Path (Get-Location).Path ".minecraft")
}

function Copy-IfPresent {
    param(
        [string]$Source,
        [string]$Destination
    )
    if (Test-Path -LiteralPath $Source) {
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        Copy-Item -Path (Join-Path $Source "*") -Destination $Destination -Recurse -Force
        return $true
    }
    return $false
}

$resolvedRunDirectory = Resolve-DefaultRunDirectory
$modsOutputDir = Join-Path $resolvedRunDirectory "mods"
New-Item -ItemType Directory -Force -Path $modsOutputDir | Out-Null

$sourceRoot = Join-Path $ScriptDir "mods"
$packagedCount = 0
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$managedListPath = Join-Path $modsOutputDir ".minecraft-native-mods"
$previousPackages = @()
if (Test-Path -LiteralPath $managedListPath) {
    $previousPackages = Get-Content -LiteralPath $managedListPath |
        Where-Object { $_ -match '^[A-Za-z0-9_.-]+\.zip$' }
}
foreach ($packageName in $previousPackages) {
    $packagePath = Join-Path $modsOutputDir $packageName
    if (Test-Path -LiteralPath $packagePath) {
        Remove-Item -LiteralPath $packagePath -Force
    }
}
$managedPackages = @()

foreach ($modDir in Get-ChildItem -LiteralPath $sourceRoot -Directory) {
    $manifestPath = Join-Path $modDir.FullName "mod.json"
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        continue
    }

    $nativeExtensions = @(".cpp", ".hpp", ".c", ".h")
    $nativeSources = Get-ChildItem -LiteralPath $modDir.FullName -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $nativeExtensions -contains $_.Extension.ToLowerInvariant() }
    if ($nativeSources -and $nativeSources.Count -gt 0) {
        throw ("Lua mod folder must not contain native sources: " + $modDir.FullName + " (" + $nativeSources[0].Name + ")")
    }
    $shaderSources = Get-ChildItem -LiteralPath $modDir.FullName -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { @(".glsl", ".vert", ".frag", ".vsh", ".fsh") -contains $_.Extension.ToLowerInvariant() }
    if ((Test-Path -LiteralPath (Join-Path $modDir.FullName "shaders")) -or
        ($shaderSources -and $shaderSources.Count -gt 0)) {
        throw ("Lua mods cannot package shaders: " + $modDir.FullName)
    }

    $staging = Join-Path $modsOutputDir (".pkg-stage-" + $modDir.Name)
    if (Test-Path -LiteralPath $staging) {
        Remove-Item -LiteralPath $staging -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $staging | Out-Null

    $manifestText = Get-Content -LiteralPath $manifestPath -Raw
    [System.IO.File]::WriteAllText((Join-Path $staging "mod.json"), $manifestText, $utf8NoBom)

    [void](Copy-IfPresent -Source (Join-Path $modDir.FullName "scripts") -Destination (Join-Path $staging "scripts"))
    [void](Copy-IfPresent -Source (Join-Path $modDir.FullName "assets") -Destination (Join-Path $staging "assets"))
    [void](Copy-IfPresent -Source (Join-Path $modDir.FullName "resources") -Destination (Join-Path $staging "resources"))
    [void](Copy-IfPresent -Source (Join-Path $modDir.FullName "lang") -Destination (Join-Path $staging "lang"))

    $zipPath = Join-Path $modsOutputDir ($modDir.Name + ".zip")
    if (Test-Path -LiteralPath $zipPath) {
        Remove-Item -LiteralPath $zipPath -Force
    }
    Compress-Archive -Path (Join-Path $staging "*") -DestinationPath $zipPath -CompressionLevel Optimal -Force
    Remove-Item -LiteralPath $staging -Recurse -Force
    $packagedCount += 1
    $managedPackages += ($modDir.Name + ".zip")

    if (-not $Quiet) {
        Write-Host ("Packaged Lua mod zip: " + $zipPath)
    }
}
[System.IO.File]::WriteAllLines($managedListPath, $managedPackages, $utf8NoBom)

if (-not $Quiet) {
    if ($packagedCount -eq 0) {
        Write-Host "No Lua mod packages found yet. Add mods/<id>/mod.json plus scripts/ to package a mod."
    } else {
        Write-Host ("Lua mod packages are ready in: " + $modsOutputDir)
    }
}
