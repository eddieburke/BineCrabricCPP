# Omega (maximum optimization) full build of minecraft_native.
#
# Usage:
#   .\build-omega.ps1              # configure if needed, then optimized release build
#   .\build-omega.ps1 -Clean       # wipe build dir and rebuild from scratch
#   .\build-omega.ps1 -BuildType Debug -BuildDir build-debug  # true debug build with symbols
#   .\build-omega.ps1 -Jobs 12     # limit parallel compile jobs
#   .\build-omega.ps1 -Lto          # opt-in link-time optimization
#   .\build-omega.ps1 -NoNativeCpu # portable binary (no -march=native / AVX tuning)
#   .\build-omega.ps1 -RunTests          # build minecraft_omega_tests and run ctest
#   .\build-omega.ps1 -Target Client     # build minecraft_native.exe only
#   .\build-omega.ps1 -Target Server     # build minecraft_server.exe only
#   .\build-client.ps1 / .\build-server.ps1  # thin wrappers (build + optional -Run)
#
# Optimizations enabled by default for Release builds:
#   CMAKE_BUILD_TYPE=Release        -> -O3 -DNDEBUG
#   -march=native -mtune=native -mprefer-vector-width=128 (see -NoNativeCpu)
#   -funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections
#   -g                              -> DWARF debug symbols (profiling / crash backtraces)
#   -Wl,--gc-sections               -> dead code / data elimination at link
#

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

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "cmake not found in PATH."
    exit 1
}
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    Write-Error "ninja not found in PATH."
    exit 1
}

$CmakeExe = "cmake"
$NinjaExe = "ninja"
$CtestExe = "ctest"

function Test-NeedsConfigure {
    param(
        [string]$BuildDirPath,
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
    $buildType = $null
    $ipo = $null
    $cxxFlagsRelease = $null
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -like "CMAKE_GENERATOR:INTERNAL=*") {
            $generator = $line.Substring("CMAKE_GENERATOR:INTERNAL=".Length)
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
}

Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir

$CmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_MAKE_PROGRAM=$NinjaExe",
    "-DCMAKE_BUILD_TYPE=$BuildType"
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
    Write-Host "LTO enabled"
    $CmakeArgs += "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
}

$OmegaCxxRelease = ""
if ($BuildType -eq "Release") {
    $OmegaCxx = "-g -funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-semantic-interposition -fmerge-all-constants"
    $OmegaLink = "-Wl,--gc-sections"
    if ($UseLto) {
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
    -ExpectedBuildType $BuildType `
    -UseLto $UseLto `
    -ExpectedCxxFlagsRelease $OmegaCxxRelease
if ($NeedsConfigure) {
    Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
    Remove-StaleNinjaRestatFile -BuildDirPath (Join-Path $ScriptDir $BuildDir)
    Write-Host "Configuring $BuildDir (Ninja, $BuildType) ..."
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
        $testTargets = @("minecraft_omega_tests")
        Write-Host "Building $($testTargets -join ', ') (-j $Jobs) ..."
        & $CmakeExe --build $BuildDir --target @testTargets -j $Jobs
        if ($LASTEXITCODE -ne 0) {
            $exitCode = $LASTEXITCODE
        } else {
            $buildDirAbs = Join-Path $ScriptDir $BuildDir
            Write-Host "Running ctest in $BuildDir ..."
            Push-Location $buildDirAbs
            try {
                & $CtestExe --output-on-failure
                if ($LASTEXITCODE -ne 0) {
                    $exitCode = $LASTEXITCODE
                }
            } finally {
                Pop-Location
            }
        }
    }
}

Write-Host ("Finished in {0:N1}s (exit $exitCode)" -f $sw.Elapsed.TotalSeconds)
exit $exitCode

} finally {
    Release-BuildOmegaLock
}
