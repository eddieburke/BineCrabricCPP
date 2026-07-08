# Omega (maximum optimization) full build of minecraft_native using a bundled MinGW GCC toolchain.
#
# On first run this script downloads GCC, CMake, Ninja, and audio/zlib deps into ./toolchain/
# (relative to this script). No system MSYS2 or Visual Studio compiler is required.
#
# Usage:
#   .\build-omega.ps1              # configure if needed, then optimized release build
#   .\build-omega.ps1 -Clean       # wipe build dir and rebuild from scratch
#   .\build-omega.ps1 -BuildType Debug -BuildDir build-debug  # true debug build with symbols
#   .\build-omega.ps1 -Jobs 12     # limit parallel compile jobs
#   .\build-omega.ps1 -Lto          # opt-in link-time optimization (often fails on MinGW GCC 15)
#   .\build-omega.ps1 -NoNativeCpu # portable binary (no -march=native / AVX tuning)
#   .\build-omega.ps1 -SkipModPackaging  # skip generating runtime mod zips
#   .\build-omega.ps1 -RunTests          # build minecraft_omega_tests and run ctest
#   .\build-omega.ps1 -Target Client     # build minecraft_native.exe only
#   .\build-omega.ps1 -Target Server     # build minecraft_server.exe only
#   .\build-client.ps1 / .\build-server.ps1  # thin wrappers (build + optional -Run)
#
# Optimizations enabled by default for Release builds (bundled GCC / Ninja):
#   CMAKE_BUILD_TYPE=Release        -> -O3 -DNDEBUG
#   -march=native -mtune=native -mprefer-vector-width=128 (see -NoNativeCpu)
#   -funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections
#   -g                              -> DWARF debug symbols (profiling / crash backtraces)
#   -Wl,--gc-sections               -> dead code / data elimination at link
#
# LTO (-Lto) is opt-in only: MinGW GCC 15 cannot link this codebase with -flto reliably.
# Intentionally omitted: -ffast-math � golden tests depend on stable FP.

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
    [ValidateSet("All", "Client", "Server")]
    [string]$Target = "All"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$ToolchainDir = Join-Path $ScriptDir "toolchain"
$MingwRoot = Join-Path $ToolchainDir "mingw64"
$DownloadsDir = Join-Path $ToolchainDir "_downloads"
$ToolsDir = Join-Path $ToolchainDir "tools"
$ManifestPath = Join-Path $ToolchainDir "manifest.json"
$MingwBin = Join-Path $MingwRoot "bin"
$GppExe = Join-Path $MingwBin "g++.exe"
$CmakeExe = Join-Path $MingwBin "cmake.exe"
$NinjaExe = Join-Path $MingwBin "ninja.exe"
$CtestExe = Join-Path $MingwBin "ctest.exe"
$DepsMarker = Join-Path $ToolchainDir ".deps-installed"

$RelToolchainRoot = "toolchain/mingw64"
$RelGpp = "$RelToolchainRoot/bin/g++.exe"
$RelNinja = "$RelToolchainRoot/bin/ninja.exe"

function Get-Manifest {
    if (-not (Test-Path -LiteralPath $ManifestPath)) {
        Write-Error "Missing toolchain manifest: $ManifestPath"
        exit 1
    }
    return Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
}

function Ensure-Directory {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Force -Path $Path | Out-Null
    }
}

function Download-File {
    param(
        [string]$Url,
        [string]$Destination
    )

    Ensure-Directory -Path (Split-Path -Parent $Destination)
    if (Test-Path -LiteralPath $Destination) {
        $existing = Get-Item -LiteralPath $Destination
        if ($existing.Length -gt 0) {
            return
        }
        Remove-Item -LiteralPath $Destination -Force
    }

    Write-Host "Downloading $(Split-Path -Leaf $Destination) ..."
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        & curl.exe -L --fail --retry 3 --retry-delay 2 -o $Destination $Url
        if ($LASTEXITCODE -ne 0) {
            Write-Error "curl failed ($LASTEXITCODE) for $Url"
            exit 1
        }
        return
    }

    Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
}

function Ensure-ZstdTool {
    $manifest = Get-Manifest
    $zstdExe = Join-Path $ToolsDir "zstd.exe"
    if (Test-Path -LiteralPath $zstdExe) {
        return $zstdExe
    }

    Ensure-Directory -Path $ToolsDir
    $archive = Join-Path $DownloadsDir $manifest.zstd.archive
    Download-File -Url $manifest.zstd.url -Destination $archive

    $extractRoot = Join-Path $ToolsDir "zstd-extract"
    if (Test-Path -LiteralPath $extractRoot) {
        Remove-Item -LiteralPath $extractRoot -Recurse -Force
    }
    Expand-Archive -LiteralPath $archive -DestinationPath $extractRoot -Force

    $sourceExe = Join-Path $extractRoot $manifest.zstd.exe_subpath
    if (-not (Test-Path -LiteralPath $sourceExe)) {
        Write-Error "zstd.exe not found in $($manifest.zstd.archive)"
        exit 1
    }
    Copy-Item -LiteralPath $sourceExe -Destination $zstdExe -Force
    return $zstdExe
}

function Merge-Ucrt64IntoMingw64 {
    param(
        [string]$StageDir,
        [string]$DestRoot
    )

    $ucrtRoot = Join-Path $StageDir "ucrt64"
    if (-not (Test-Path -LiteralPath $ucrtRoot)) {
        Write-Error "MSYS2 package did not contain ucrt64/: $StageDir"
        exit 1
    }

    foreach ($subdir in @("bin", "include", "lib", "share")) {
        $from = Join-Path $ucrtRoot $subdir
        if (-not (Test-Path -LiteralPath $from)) {
            continue
        }
        $to = Join-Path $DestRoot $subdir
        Ensure-Directory -Path $to
        Copy-Item -Path (Join-Path $from "*") -Destination $to -Recurse -Force
    }
}

function Install-MsysUcrtPackage {
    param(
        [string]$Url,
        [string]$PackageName,
        [string]$MingwDest,
        [string]$ZstdTool
    )

    $fileName = Split-Path -Leaf $Url
    $archivePath = Join-Path $DownloadsDir $fileName
    Download-File -Url $Url -Destination $archivePath

    $tarPath = Join-Path $DownloadsDir ($fileName + ".tar")
    if (Test-Path -LiteralPath $tarPath) {
        Remove-Item -LiteralPath $tarPath -Force
    }

    & $ZstdTool -d $archivePath -o $tarPath -f
    if ($LASTEXITCODE -ne 0) {
        Write-Error "zstd failed extracting $PackageName"
        exit 1
    }

    $stageDir = Join-Path $DownloadsDir ("stage-" + $PackageName)
    if (Test-Path -LiteralPath $stageDir) {
        Remove-Item -LiteralPath $stageDir -Recurse -Force
    }
    Ensure-Directory -Path $stageDir

    tar -xf $tarPath -C $stageDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "tar failed extracting $PackageName"
        exit 1
    }

    Merge-Ucrt64IntoMingw64 -StageDir $stageDir -DestRoot $MingwDest
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}

function Test-ToolchainDepsPresent {
    param([string]$MingwDest)

    $requiredLibs = @("libz.a", "libogg.a", "libvorbis.a", "libvorbisfile.a")
    foreach ($lib in $requiredLibs) {
        if (-not (Test-Path -LiteralPath (Join-Path $MingwDest "lib\$lib"))) {
            return $false
        }
    }
    return $true
}

function Ensure-ToolchainDeps {
    param([string]$MingwDest)

    if ((Test-Path -LiteralPath $DepsMarker) -or (Test-ToolchainDepsPresent -MingwDest $MingwDest)) {
        return
    }

    Write-Host "Installing bundled zlib/vorbis deps into toolchain/mingw64 ..."
    $manifest = Get-Manifest
    $zstdTool = Ensure-ZstdTool
    Ensure-Directory -Path $MingwDest

    foreach ($pkg in $manifest.msys2_ucrt_packages) {
        Write-Host "  -> $($pkg.name)"
        Install-MsysUcrtPackage -Url $pkg.url -PackageName $pkg.name -MingwDest $MingwDest -ZstdTool $zstdTool
    }

    Set-Content -LiteralPath $DepsMarker -Value ("installed " + (Get-Date -Format "o")) -Encoding ASCII
}

function Test-LocalToolchainPresent {
    return (Test-Path -LiteralPath $GppExe) -and
        (Test-Path -LiteralPath $CmakeExe) -and
        (Test-Path -LiteralPath $NinjaExe)
}

function Ensure-BundledToolchain {
    if (Test-LocalToolchainPresent) {
        Ensure-ToolchainDeps -MingwDest $MingwRoot
        Write-Host "Using downloaded toolchain: $RelToolchainRoot"
        return
    }

    $manifest = Get-Manifest
    Ensure-Directory -Path $DownloadsDir

    $archivePath = Join-Path $DownloadsDir $manifest.winlibs.archive
    Download-File -Url $manifest.winlibs.url -Destination $archivePath

    $staging = Join-Path $ToolchainDir "_staging"
    if (Test-Path -LiteralPath $staging) {
        Remove-Item -LiteralPath $staging -Recurse -Force
    }
    Ensure-Directory -Path $staging

    Write-Host "Extracting bundled GCC toolchain ..."
    Expand-Archive -LiteralPath $archivePath -DestinationPath $staging -Force

    $extractedMingw = Join-Path $staging "mingw64"
    if (-not (Test-Path -LiteralPath $extractedMingw)) {
        $candidate = Get-ChildItem -LiteralPath $staging -Directory | Where-Object { Test-Path (Join-Path $_.FullName "bin\g++.exe") } | Select-Object -First 1
        if ($null -eq $candidate) {
            Write-Error "WinLibs archive did not contain mingw64/bin/g++.exe"
            exit 1
        }
        $extractedMingw = $candidate.FullName
    }

    if (Test-Path -LiteralPath $MingwRoot) {
        Remove-Item -LiteralPath $MingwRoot -Recurse -Force
    }
    Move-Item -LiteralPath $extractedMingw -Destination $MingwRoot
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue

    if (-not (Test-Path -LiteralPath $GppExe)) {
        Write-Error "Bundled toolchain install failed: $GppExe"
        exit 1
    }

    Ensure-ToolchainDeps -MingwDest $MingwRoot
    Write-Host "Bundled toolchain ready: $MingwRoot"
}

function Set-BundledToolchainEnvironment {
    param([string]$MingwBinDir)

    $filtered = @()
    if ($env:PATH) {
        $filtered = $env:PATH -split ';' | Where-Object {
            $_ -ne "" -and
            $_ -notmatch '(?i)msys64' -and
            $_ -notmatch '(?i)\\mingw64\\bin' -and
            $_ -notmatch '(?i)Microsoft Visual Studio'
        }
    }

    $env:PATH = ($MingwBinDir + ';' + ($filtered -join ';'))
    $env:CXX = Join-Path $MingwBinDir "g++.exe"
    Remove-Item Env:\CC -ErrorAction SilentlyContinue
}

$BuildOmegaLockPath = Join-Path $ScriptDir ".build-omega.lock"
$BuildOmegaLockStream = $null

function Remove-StaleBuildOmegaLockFile {
    param([string]$LockPath)
    if (-not (Test-Path -LiteralPath $LockPath)) {
        return
    }
    $remove = $false
    try {
        $content = Get-Content -LiteralPath $LockPath -ErrorAction Stop | Select-Object -First 1
        if ($null -eq $content -or $content -eq "") {
            $remove = $true
        } else {
            $lockPid = 0
            if ([int]::TryParse($content, [ref]$lockPid)) {
                $proc = Get-Process -Id $lockPid -ErrorAction SilentlyContinue
                if ($null -eq $proc) {
                    $remove = $true
                }
            } else {
                $remove = $true
            }
        }
    } catch {
        $remove = $true
    }
    if ($remove) {
        Remove-Item -LiteralPath $LockPath -Force -ErrorAction SilentlyContinue
    }
}

function Release-BuildOmegaLock {
    if ($null -ne $script:BuildOmegaLockStream) {
        try {
            $script:BuildOmegaLockStream.Close()
        } catch {}
        try {
            $script:BuildOmegaLockStream.Dispose()
        } catch {}
        $script:BuildOmegaLockStream = $null
    }
    if (Test-Path -LiteralPath $script:BuildOmegaLockPath) {
        Remove-Item -LiteralPath $script:BuildOmegaLockPath -Force -ErrorAction SilentlyContinue
    }
}

function Get-ExternalBuildProcesses {
    param(
        [string]$BuildDirName,
        [string]$ScriptDirPath
    )

    $buildDirAbs = (Join-Path $ScriptDirPath $BuildDirName).Replace("\", "/")
    $buildDirLeaf = $BuildDirName.Replace("\", "/")
    $matches = @()

    Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -eq "cmake.exe" -or $_.Name -eq "ninja.exe" } |
        ForEach-Object {
            $cmd = $_.CommandLine
            if (-not $cmd) {
                return
            }
            $cmdNorm = $cmd.Replace("\", "/")
            if ($cmdNorm -like "*$buildDirAbs*" -or $cmdNorm -like "*--build*$buildDirLeaf*") {
                $matches += $_
            }
        }

    return $matches
}

function Assert-BuildDirAvailable {
    param(
        [string]$BuildDirName,
        [string]$ScriptDirPath
    )

    $procs = Get-ExternalBuildProcesses -BuildDirName $BuildDirName -ScriptDirPath $ScriptDirPath
    if ($procs.Count -eq 0) {
        return
    }

    $details = ($procs | ForEach-Object {
        "  PID $($_.ProcessId): $($_.Name)"
    }) -join [Environment]::NewLine

    Write-Error @"
Another cmake/ninja build is already using '$BuildDirName'.
Wait for it to finish, or stop those processes, then retry.
$details
"@
    exit 1
}

function Remove-StaleNinjaRestatFile {
    param([string]$BuildDirPath)

    $restat = Join-Path $BuildDirPath ".ninja_log.restat"
    if (Test-Path -LiteralPath $restat) {
        Remove-Item -LiteralPath $restat -Force -ErrorAction SilentlyContinue
    }
}

function Normalize-ToolPath {
    param([string]$Path)
    if (-not $Path) {
        return ""
    }
    return $Path.Replace("\", "/").ToLowerInvariant()
}

function Test-NeedsConfigure {
    param(
        [string]$BuildDirPath,
        [string]$ExpectedCompiler,
        [string]$ExpectedToolchainRoot,
        [string]$ExpectedBuildType,
        [bool]$UseLto,
        [string]$ExpectedCxxFlagsRelease = ""
    )

    $cachePath = Join-Path $BuildDirPath "CMakeCache.txt"
    if (-not (Test-Path -LiteralPath $cachePath)) {
        return $true
    }
    if (-not (Test-Path -LiteralPath (Join-Path $BuildDirPath "build.ninja"))) {
        return $true
    }

    $generator = $null
    $compiler = $null
    $toolchainRoot = $null
    $buildType = $null
    $ipo = $null
    $cxxFlagsRelease = $null
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -like "CMAKE_GENERATOR:INTERNAL=*") {
            $generator = $line.Substring("CMAKE_GENERATOR:INTERNAL=".Length)
        } elseif ($line -like "CMAKE_CXX_COMPILER:FILEPATH=*") {
            $compiler = $line.Substring("CMAKE_CXX_COMPILER:FILEPATH=".Length)
        } elseif ($line -like "MINECRAFT_TOOLCHAIN_ROOT:PATH=*") {
            $toolchainRoot = $line.Substring("MINECRAFT_TOOLCHAIN_ROOT:PATH=".Length)
        } elseif ($line -like "CMAKE_BUILD_TYPE:STRING=*") {
            $buildType = $line.Substring("CMAKE_BUILD_TYPE:STRING=".Length)
        } elseif ($line -like "CMAKE_INTERPROCEDURAL_OPTIMIZATION:BOOL=*") {
            $ipo = $line.Substring("CMAKE_INTERPROCEDURAL_OPTIMIZATION:BOOL=".Length)
        } elseif ($line -like "CMAKE_CXX_FLAGS_RELEASE:STRING=*") {
            $cxxFlagsRelease = $line.Substring("CMAKE_CXX_FLAGS_RELEASE:STRING=".Length).Trim()
        }
    }

    if ($generator -ne "Ninja") {
        return $true
    }
    if ((Normalize-ToolPath $compiler) -ne (Normalize-ToolPath $ExpectedCompiler)) {
        return $true
    }
    if ($toolchainRoot -and ((Normalize-ToolPath $toolchainRoot) -ne (Normalize-ToolPath $ExpectedToolchainRoot))) {
        return $true
    }
    if ($buildType -ne $ExpectedBuildType) {
        return $true
    }

    $cachedLto = ($ipo -eq "ON")
    if ($cachedLto -ne $UseLto) {
        return $true
    }

    if ($ExpectedBuildType -eq "Release" -and $ExpectedCxxFlagsRelease -ne "") {
        $expectedNorm = ($ExpectedCxxFlagsRelease -replace '\s+', ' ').Trim()
        $cachedNorm = if ($cxxFlagsRelease) { ($cxxFlagsRelease -replace '\s+', ' ').Trim() } else { "" }
        if ($cachedNorm -ne $expectedNorm) {
            return $true
        }
    }

    return $false
}

Ensure-BundledToolchain
Set-BundledToolchainEnvironment -MingwBinDir $MingwBin

if (-not (Test-Path -LiteralPath $CmakeExe)) {
    Write-Error "Bundled cmake not found at $CmakeExe"
    exit 1
}
if (-not (Test-Path -LiteralPath $NinjaExe)) {
    Write-Error "Bundled ninja not found at $NinjaExe"
    exit 1
}

Remove-StaleBuildOmegaLockFile -LockPath $BuildOmegaLockPath

try {
    try {
        $BuildOmegaLockStream = [System.IO.File]::Open(
            $BuildOmegaLockPath,
            [System.IO.FileMode]::CreateNew,
            [System.IO.FileAccess]::ReadWrite,
            [System.IO.FileShare]::None)
        $pidBytes = [System.Text.Encoding]::ASCII.GetBytes("$PID`n")
        $BuildOmegaLockStream.Write($pidBytes, 0, $pidBytes.Length)
        $BuildOmegaLockStream.Flush()
    } catch {
        Write-Error "Already in use"
        exit 1
    }

if ($Jobs -le 0) {
    # Cap default parallelism: large link steps and many TUs can OOM if -j is too high.
    $Jobs = [Math]::Min(12, [Math]::Max(1, [Environment]::ProcessorCount - 2))
}

if ($Clean -and (Test-Path $BuildDir)) {
    Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
    Write-Host "Removing $BuildDir ..."
    Remove-Item -Recurse -Force $BuildDir
    $CcacheExe = Join-Path $MingwBin "ccache.exe"
    if (Test-Path -LiteralPath $CcacheExe) {
        Write-Host "Clearing ccache (stale LTO objects) ..."
        & $CcacheExe -C
    }
}

Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir

Write-Host "Toolchain: $RelToolchainRoot"
Write-Host "Compiler:  $RelGpp"

$CmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_MAKE_PROGRAM=$NinjaExe",
    "-DCMAKE_CXX_COMPILER=$GppExe",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DMINECRAFT_TOOLCHAIN_ROOT=$RelToolchainRoot"
)

$UseLto = $Lto -and (-not $NoLto)
if ($Lto -and $NoLto) {
    Write-Error "-Lto and -NoLto cannot be used together."
    exit 1
}
if ($UseLto -and $BuildType -ne "Release") {
    Write-Error "-Lto is only supported with -BuildType Release."
    exit 1
}

if ($UseLto) {
    Write-Host "LTO enabled (opt-in; may fail to link on MinGW GCC 15)"
    $CmakeArgs += "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
    $CmakeArgs += "-DCMAKE_CXX_COMPILER_LAUNCHER="
    $env:CCACHE_DISABLE = "1"
}

$OmegaCxxRelease = ""
if ($BuildType -eq "Release") {
    $OmegaCxx = "-g -funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-semantic-interposition -fmerge-all-constants"
    $OmegaLink = "-Wl,--gc-sections"
    if ($UseLto) {
        # LTO + --gc-sections triggers duplicate .gnu.lto section errors on MinGW GCC 15.
        $OmegaLink = ""
        $OmegaCxx = $OmegaCxx + " -flto-partition=one"
    }
    if (-not $NoNativeCpu) {
        $OmegaCxx = "-march=native -mtune=native -mprefer-vector-width=128 " + $OmegaCxx
    }
    $OmegaCxxRelease = $OmegaCxx
    $CmakeArgs += "-DCMAKE_CXX_FLAGS_RELEASE=$OmegaCxx"
    if ($OmegaLink -ne "") {
        $CmakeArgs += "-DCMAKE_EXE_LINKER_FLAGS_RELEASE=$OmegaLink"
    }
}

$NeedsConfigure = Test-NeedsConfigure `
    -BuildDirPath (Join-Path $ScriptDir $BuildDir) `
    -ExpectedCompiler $GppExe `
    -ExpectedToolchainRoot (Join-Path $ScriptDir $RelToolchainRoot.Replace("/", "\")) `
    -ExpectedBuildType $BuildType `
    -UseLto $UseLto `
    -ExpectedCxxFlagsRelease $OmegaCxxRelease
if ($NeedsConfigure) {
    Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
    Remove-StaleNinjaRestatFile -BuildDirPath (Join-Path $ScriptDir $BuildDir)
    if ($BuildType -eq "Release") {
        Write-Host "Configuring $BuildDir (Ninja + bundled GCC, Release, omega flags) ..."
    } else {
        Write-Host "Configuring $BuildDir (Ninja + bundled GCC, $BuildType) ..."
    }
    & $CmakeExe @CmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
$buildTargets = @("minecraft_native", "minecraft_server")
$buildLabel = "minecraft_native + minecraft_server"
if ($Target -eq "Client") {
    $buildTargets = @("minecraft_native")
    $buildLabel = "minecraft_native (client)"
} elseif ($Target -eq "Server") {
    $buildTargets = @("minecraft_server")
    $buildLabel = "minecraft_server"
}
if ($BuildType -eq "Release") {
    Write-Host "Omega build: $buildLabel (-j $Jobs) ..."
} else {
    Write-Host "$BuildType build: $buildLabel (-j $Jobs) ..."
}
$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $CmakeExe --build $BuildDir --target @buildTargets -j $Jobs
$exitCode = $LASTEXITCODE
$sw.Stop()

    if ($exitCode -eq 0) {
    if ($Target -eq "All" -or $Target -eq "Client") {
        $clientExe = Join-Path $BuildDir "minecraft_native.exe"
        if (Test-Path $clientExe) {
            Write-Host "Client: $clientExe"
        }
    }
    if ($Target -eq "All" -or $Target -eq "Server") {
        $serverExe = Join-Path $BuildDir "minecraft_server.exe"
        if (Test-Path $serverExe) {
            Write-Host "Server: $serverExe"
        }
    }

    if ($RunTests -and $exitCode -eq 0) {
        $testTargets = @()
        if ($Target -eq "All" -or $Target -eq "Client" -or $Target -eq "Server") {
            $testTargets += "minecraft_omega_tests"
        }
        if ($testTargets.Count -gt 0) {
        Write-Host "Building $($testTargets -join ', ') (-j $Jobs) ..."
        & $CmakeExe --build $BuildDir --target @testTargets -j $Jobs
        if ($LASTEXITCODE -ne 0) {
            $exitCode = $LASTEXITCODE
        } else {
            $buildDirAbs = Join-Path $ScriptDir $BuildDir
            Write-Host "Running ctest in $BuildDir ..."
            Push-Location $buildDirAbs
            try {
                if (-not (Test-Path -LiteralPath $CtestExe)) {
                    Write-Error "Bundled ctest not found at $CtestExe"
                    $exitCode = 1
                } else {
                    & $CtestExe --output-on-failure
                    if ($LASTEXITCODE -ne 0) {
                        $exitCode = $LASTEXITCODE
                    }
                }
            } finally {
                Pop-Location
            }
        }
        }
    }
}

Write-Host ("Finished in {0:N1}s (exit $exitCode)" -f $sw.Elapsed.TotalSeconds)
exit $exitCode

} finally {
    Release-BuildOmegaLock
}
