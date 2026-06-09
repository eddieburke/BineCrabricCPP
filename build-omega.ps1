# Omega (maximum optimization) full build of minecraft_native using Ninja.
#
# Usage:
#   .\build-omega.ps1              # configure if needed, then optimized release build
#   .\build-omega.ps1 -Clean       # wipe build dir and rebuild from scratch
#   .\build-omega.ps1 -Jobs 12     # limit parallel compile jobs
#   .\build-omega.ps1 -NoLto       # skip link-time optimization (faster link, less perf)
#   .\build-omega.ps1 -NoNativeCpu # portable binary (no -march=native / /arch:AVX2)
#   .\build-omega.ps1 -RunTests    # run ctest after a successful build
#
# Optimizations enabled (GNU/MinGW — primary toolchain for this project):
#   CMAKE_BUILD_TYPE=Release        -> -O3 -DNDEBUG (max speed, asserts stripped)
#   CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -> -flto whole-program optimization
#   -march=native -mtune=native     -> emit instructions for this CPU (see -NoNativeCpu)
#   -funroll-loops                  -> aggressive loop unrolling
#   -fomit-frame-pointer            -> free a register in leaf hot paths
#   -ffunction-sections -fdata-sections -> per-symbol sections for linker GC
#   -Wl,--gc-sections               -> dead code / data elimination at link
#   -fno-semantic-interposition     -> stronger cross-TU inlining under LTO
#   -fmerge-all-constants           -> fold duplicate compile-time constants
#
# MSVC (when cl.exe is the default and g++ is absent):
#   /O2 /Ob2 /Oi /Ot                -> max speed, inlining, intrinsic expansion
#   /GL + CMAKE_INTERPROCEDURAL_OPTIMIZATION -> link-time code generation (/LTCG)
#   /arch:AVX2                      -> SIMD for this generation (see -NoNativeCpu)
#
# Intentionally omitted: -ffast-math / /fp:fast — golden tests depend on stable FP.

param(
    [string]$BuildDir = "build-omega",
    [int]$Jobs = 0,
    [switch]$Clean,
    [switch]$NoLto,
    [switch]$NoNativeCpu,
    [switch]$RunTests
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

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

function Get-CompilerFamily {
    $gpp = Get-Command g++ -ErrorAction SilentlyContinue
    if ($gpp) {
        return "GNU"
    }
    $cl = Get-Command cl -ErrorAction SilentlyContinue
    if ($cl) {
        return "MSVC"
    }
    return "GNU"
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
    $Jobs = [Math]::Max(1, [Environment]::ProcessorCount - 2)
}

if ($Clean -and (Test-Path $BuildDir)) {
    Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
    Write-Host "Removing $BuildDir ..."
    Remove-Item -Recurse -Force $BuildDir
}

Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir

$CompilerFamily = Get-CompilerFamily
Write-Host "Compiler family: $CompilerFamily"

$CmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release"
)

if (-not $NoLto) {
    $CmakeArgs += "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON"
}

if ($CompilerFamily -eq "GNU") {
    $OmegaCxx = "-funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-semantic-interposition -fmerge-all-constants"
    $OmegaLink = "-Wl,--gc-sections"
    if (-not $NoNativeCpu) {
        # -march=native enables AVX2, which raises __BIGGEST_ALIGNMENT__ to 32 and lets
        # GCC copy 32-byte structs (e.g. BiomeInfo) with a 256-bit *aligned* vmovdqa to a
        # stack temp. The Win64 ABI only guarantees 16-byte stack alignment, so that move
        # faults (#GP -> 0xC0000005, fault address 0xffffffffffffffff) ~50% of the time.
        # Capping vector width at 128 bits keeps AVX scalar/FMA but emits 16-byte moves to
        # the 16-aligned stack, eliminating the crash. (Repro: enter the Nether.)
        $OmegaCxx = "-march=native -mtune=native -mprefer-vector-width=128 " + $OmegaCxx
    }
    $CmakeArgs += "-DCMAKE_CXX_FLAGS_RELEASE=$OmegaCxx"
    $CmakeArgs += "-DCMAKE_EXE_LINKER_FLAGS_RELEASE=$OmegaLink"
} else {
    $OmegaCxx = "/O2 /Ob2 /Oi /Ot"
    if (-not $NoNativeCpu) {
        $OmegaCxx = $OmegaCxx + " /arch:AVX2"
    }
    $CmakeArgs += "-DCMAKE_CXX_FLAGS_RELEASE=$OmegaCxx"
}

$NeedsConfigure = -not (Test-Path (Join-Path $BuildDir "build.ninja"))
if ($NeedsConfigure) {
    Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
    Remove-StaleNinjaRestatFile -BuildDirPath (Join-Path $ScriptDir $BuildDir)
    Write-Host "Configuring $BuildDir (Ninja, Release, omega flags) ..."
    & cmake @CmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Assert-BuildDirAvailable -BuildDirName $BuildDir -ScriptDirPath $ScriptDir
Write-Host "Omega build: minecraft_native (-j $Jobs) ..."
$sw = [System.Diagnostics.Stopwatch]::StartNew()
cmake --build $BuildDir --target minecraft_native -j $Jobs
$exitCode = $LASTEXITCODE
$sw.Stop()

if ($exitCode -eq 0) {
    $exe = Join-Path $BuildDir "minecraft_native.exe"
    if (Test-Path $exe) {
        Write-Host "Output: $exe"
    }
    if ($RunTests) {
        Write-Host "Building test targets ..."
        cmake --build $BuildDir -j $Jobs
        if ($LASTEXITCODE -ne 0) {
            $exitCode = $LASTEXITCODE
        } else {
            Write-Host "Running tests (ctest) ..."
            ctest --test-dir $BuildDir --output-on-failure
            if ($LASTEXITCODE -ne 0) {
                $exitCode = $LASTEXITCODE
            }
        }
    }
}

Write-Host ("Finished in {0:N1}s (exit $exitCode)" -f $sw.Elapsed.TotalSeconds)
exit $exitCode

} finally {
    Release-BuildOmegaLock
}
