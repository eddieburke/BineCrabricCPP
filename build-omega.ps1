# Omega (maximum optimization) full build of minecraft_native using a bundled MinGW GCC toolchain.
# This is the ONLY build script in native/ - build-client.ps1, build-server.ps1,
# clean-omega.ps1, format-omega.ps1, package-mods.ps1, and build-gui.ps1 have all been
# folded into this file as flags/modes (see history note near the bottom of this block).
#
# ===== AGENT QUICK-READ (read this block, skip the rest unless you need detail) =====
#   -BuildType Release (default)  -> optimized, SMALL, no debug symbols (true release/ship build)
#   -BuildType Debug               -> -O0, full DWARF symbols, no optimization (true debug build)
#   -BuildType RelWithDebInfo      -> optimized + full DWARF symbols (profiling/crash-backtrace build)
#   -KeepDebugSymbols               -> keep -g on a Release build (old default behavior; big exe)
#   -StripSymbols                   -> run strip.exe on the output exe(s) after linking, any BuildType
#   -Gui                            -> interactive menu (was the standalone build-gui.ps1; now merged here)
#   -Run                            -> launch the built exe when the build succeeds (was build-client/-server.ps1)
#   -NoGui                          -> with -Run -Target Server, pass "nogui" to the server (headless)
#   -CleanOnly                      -> wipe build dir + lock file and exit, no build (was clean-omega.ps1)
#   -Format                         -> run clang-format over src/ and tests/ and exit (was format-omega.ps1)
#   -ModId <id> / -NoModDeploy      -> mod packaging controls (was package-mods.ps1; runs by default, see -SkipModPackaging)
#   Mod packaging and shaderpack sync run BY DEFAULT on every successful build (see -SkipModPackaging / -SkipResourceSync to opt out).
#   Everything else below is unchanged from before.
# =======================================================================================
#
# On first run this script downloads GCC, CMake, Ninja, and audio/zlib deps into ./toolchain/
# (relative to this script). No system MSYS2 or Visual Studio compiler is required.
#
# Usage:
#   .\build-omega.ps1                    # configure if needed, then optimized + stripped release build
#   .\build-omega.ps1 -BuildType Debug   # full debug build with symbols, no optimization
#   .\build-omega.ps1 -BuildType RelWithDebInfo  # optimized build that keeps symbols (profiling)
#   .\build-omega.ps1 -KeepDebugSymbols  # Release build, but keep -g (old default; large exe)
#   .\build-omega.ps1 -StripSymbols      # strip symbol table from the output exe(s) post-link
#   .\build-omega.ps1 -Clean             # wipe build dir and rebuild from scratch
#   .\build-omega.ps1 -CleanOnly         # wipe build dir + lock file, then exit (no build)
#   .\build-omega.ps1 -Format            # clang-format src/ and tests/, then exit (no build)
#   .\build-omega.ps1 -Target Client -Run              # build the client and launch it
#   .\build-omega.ps1 -Target Server -Run -NoGui       # build the server and launch it headless
#   .\build-omega.ps1 -Target Client -Run -- --username Player  # extra args after -- go to the launched exe
#   .\build-omega.ps1 -ModId camera -NoModDeploy  # package only one mod, skip deploying it to %APPDATA%
#   .\build-omega.ps1 -Jobs 12           # limit parallel compile jobs
#   .\build-omega.ps1 -Lto               # opt-in link-time optimization (often fails on MinGW GCC 15)
#   .\build-omega.ps1 -NoNativeCpu       # portable binary (no -march=native / AVX tuning)
#   .\build-omega.ps1 -SkipModPackaging  # skip generating runtime mod zips
#   .\build-omega.ps1 -SkipResourceSync  # skip mirroring resources/ next to the built binary
#   .\build-omega.ps1 -RunTests          # build minecraft_omega_tests and run ctest
#   .\build-omega.ps1 -Target Client     # build minecraft_native.exe only
#   .\build-omega.ps1 -Target Server     # build minecraft_server.exe only
#   .\build-omega.ps1 -Gui               # interactive menu-driven build assistant (no flags needed)
#   .\build.bat                          # double-clickable launcher; forces pwsh 7 if installed, opens -Gui
#
# Optimizations enabled by default for Release builds (bundled GCC / Ninja):
#   CMAKE_BUILD_TYPE=Release        -> -O3 -DNDEBUG
#   -march=native -mtune=native -mprefer-vector-width=128 (see -NoNativeCpu)
#   -funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections
#   (no -g by default any more - pass -KeepDebugSymbols or use -BuildType RelWithDebInfo
#    if you need DWARF symbols on an optimized build)
#   -Wl,--gc-sections               -> dead code / data elimination at link
#
# History: Release used to always bake in -g DWARF symbols "for profiling / crash
# backtraces", which made minecraft_native.exe balloon to ~600 MB. Release is now a true
# small ship build; use -BuildType RelWithDebInfo or -KeepDebugSymbols when symbols on an
# optimized binary are actually needed.
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
    [switch]$SkipModPackaging,
    [switch]$SkipResourceSync,
    [switch]$KeepDebugSymbols,
    [switch]$StripSymbols,
    [switch]$Gui,
    [switch]$Run,
    [switch]$NoGui,
    [switch]$CleanOnly,
    [switch]$Format,
    [string]$ModId = "",
    [switch]$NoModDeploy,
    [ValidateSet("All", "Client", "Server")]
    [string]$Target = "All",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RunArgs
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
$ShaderpacksSource = Join-Path $ScriptDir "shaderpacks"
$ResourcesSource = Join-Path $ScriptDir "resources"

$RelToolchainRoot = "toolchain/mingw64"
$RelGpp = "$RelToolchainRoot/bin/g++.exe"
$RelNinja = "$RelToolchainRoot/bin/ninja.exe"
$BuildOmegaLockPath = Join-Path $ScriptDir ".build-omega.lock"

# ===== -CleanOnly (was clean-omega.ps1) =====
if ($CleanOnly) {
    $cleanTarget = Join-Path $ScriptDir $BuildDir
    Write-Host "Cleaning $BuildDir ..."
    try {
        if (Test-Path -LiteralPath $cleanTarget) {
            Write-Host "Removing $cleanTarget"
            Remove-Item -LiteralPath $cleanTarget -Recurse -Force -ErrorAction Stop
        }
        if (Test-Path -LiteralPath $BuildOmegaLockPath) {
            Write-Host "Removing $BuildOmegaLockPath"
            Remove-Item -LiteralPath $BuildOmegaLockPath -Force -ErrorAction Stop
        }
    } catch {
        # Another agent/session may hold this build dir or lock file open right now
        # (concurrent builds are expected in this repo) - fail quietly, not with a crash.
        Write-Warning "Could not fully clean $BuildDir - it may be in use by another build. $_"
        exit 1
    }
    Write-Host "Clean complete."
    exit 0
}

# ===== -Format (was format-omega.ps1) =====
# Token-dense clang-format pass. Uses native/.clang-format: ColumnLimit 0, Attach braces, 2-space indent.
if ($Format) {
    $clangFormatCandidates = @(
        "C:\Program Files\LLVM\bin\clang-format.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-format.exe"
    )
    $clangFormat = $null
    foreach ($candidate in $clangFormatCandidates) {
        if (Test-Path -LiteralPath $candidate) {
            $clangFormat = $candidate
            break
        }
    }
    if (-not $clangFormat) {
        Write-Error "clang-format not found. Install LLVM or Visual Studio LLVM tools."
        exit 1
    }

    $extensions = @("*.cpp", "*.hpp", "*.h")
    $roots = @("src", "tests")
    $files = @()
    foreach ($root in $roots) {
        $rootPath = Join-Path $ScriptDir $root
        if (-not (Test-Path -LiteralPath $rootPath)) {
            continue
        }
        foreach ($extension in $extensions) {
            $files += Get-ChildItem -Path $rootPath -Recurse -Filter $extension -File
        }
    }

    if ($files.Count -eq 0) {
        Write-Host "No C++ files found to format."
        exit 0
    }

    Write-Host ("Formatting {0} files with {1}" -f $files.Count, $clangFormat)
    $failures = 0
    foreach ($file in $files) {
        & $clangFormat -i $file.FullName 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            $failures++
            Write-Host ("clang-format failed: {0}" -f $file.FullName)
        }
    }

    if ($failures -gt 0) {
        Write-Error ("clang-format failed on {0} file(s)." -f $failures)
        exit 1
    }
    Write-Host "clang-format complete."
    exit 0
}

# ===== CLGUI (interactive menu) =====
# Was the standalone build-gui.ps1; merged in here so there's one script to maintain.
# Everything below runs and exits before any of the flag-driven build logic further down.
if ($Gui) {
    function Write-GuiRule { Write-Host ("-" * 70) -ForegroundColor DarkGray }
    function Write-GuiHeading {
        param([string]$Text)
        Write-Host ""
        Write-GuiRule
        Write-Host "  $Text" -ForegroundColor Cyan
        Write-GuiRule
    }
    function Write-GuiExplain {
        param([string[]]$Lines)
        foreach ($line in $Lines) { Write-Host "  $line" -ForegroundColor Gray }
    }
    function Confirm-GuiAction {
        param([string]$Prompt)
        Write-Host ""
        $answer = Read-Host "$Prompt [Enter = Yes, type N = No]"
        return -not ($answer -match '^(n|no)$')
    }
    function Test-GuiToolchainInstalled { return Test-Path -LiteralPath $GppExe }
    function Get-GuiFreeSpaceGB {
        try {
            $drive = (Get-Item -LiteralPath $ScriptDir).PSDrive
            return [Math]::Round($drive.Free / 1GB, 1)
        } catch { return $null }
    }
    function Get-GuiExeSizes {
        $paths = @(
            @{ Label = "Client"; Path = (Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_native.exe")) },
            @{ Label = "Server"; Path = (Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_server.exe")) }
        )
        foreach ($p in $paths) {
            if (Test-Path -LiteralPath $p.Path) {
                $sizeMB = [Math]::Round((Get-Item -LiteralPath $p.Path).Length / 1MB, 1)
                Write-Host "  $($p.Label): $($p.Path) ($sizeMB MB)"
            } else {
                Write-Host "  $($p.Label): not built yet"
            }
        }
    }
    function Show-GuiWelcome {
        Write-GuiHeading "Minecraft (Beta 1.7.3 native port) - Build Assistant"
        Write-GuiExplain @(
            "This tool turns the game's source code into a program you can run."
            "That process is called 'building' or 'compiling'."
            ""
            "You do not need to know how to program to use this. Just pick a"
            "number from the menu and this tool will do the rest."
            ""
            "The very first time you build, this tool needs a 'compiler toolkit'"
            "(the software that turns source code into a .exe file). If it is not"
            "already on this computer, it will be downloaded automatically into"
            "the 'toolchain' folder next to this script."
        )
        if (Test-GuiToolchainInstalled) {
            Write-Host ""
            Write-Host "  Status: build toolkit is already installed on this computer." -ForegroundColor Green
        } else {
            Write-Host ""
            Write-Host "  Status: build toolkit NOT installed yet." -ForegroundColor Yellow
            $freeGB = Get-GuiFreeSpaceGB
            if ($null -ne $freeGB) {
                Write-GuiExplain @("Free disk space available: $freeGB GB (about 3 GB recommended).")
            }
        }
    }
    function Show-GuiMenu {
        Write-Host ""
        Write-GuiRule
        Write-Host "  What would you like to do?" -ForegroundColor Cyan
        Write-GuiRule
        Write-Host "  [1] Build the game (Release - small, optimized, no debug symbols)"
        Write-Host "  [2] Build the game AND start playing right away"
        Write-Host "  [3] Build a DEBUG build (full symbols, no optimization - for troubleshooting crashes)"
        Write-Host "  [4] Build a PROFILING build (optimized, but keeps debug symbols - large exe)"
        Write-Host "  [5] Start completely fresh (fixes weird build problems, slower)"
        Write-Host "  [6] Build only the multiplayer server (for hosting)"
        Write-Host "  [7] Build and double-check everything works (runs the test suite)"
        Write-Host "  [8] Strip debug symbols from an already-built exe (shrink it)"
        Write-Host "  [9] Show sizes of built exe(s)"
        Write-Host "  [10] Open the folder with the finished game"
        Write-Host "  [11] Clean build folder only (no rebuild)"
        Write-Host "  [12] Format C++ source (clang-format)"
        Write-Host "  [13] Explain all this again"
        Write-Host "  [14] Exit"
        Write-Host ""
        return Read-Host "Type a number and press Enter"
    }
    function Show-GuiTroubleshooting {
        param([int]$ExitCode)
        Write-Host ""
        Write-Host "  Something went wrong (exit code $ExitCode)." -ForegroundColor Red
        Write-GuiExplain @(
            "A few common causes, in order of likelihood:"
            ""
            "  - No internet connection during the first build (the toolkit"
            "    download failed). Check your connection and try again."
            "  - Antivirus or a firewall blocked the download or the compiler."
            "  - The game or a previous build is still running. Close"
            "    minecraft_native.exe / minecraft_server.exe and try again."
            "  - Not enough free disk space (a build needs a few GB free)."
            ""
            "You can simply try the same menu option again - builds pick up"
            "where they left off instead of starting over from nothing."
        )
    }
    function Invoke-GuiBuild {
        param([string[]]$Arguments, [string]$FriendlyName)
        Write-Host ""
        Write-Host "  Starting: $FriendlyName ..." -ForegroundColor Cyan
        Write-GuiExplain @("This window will fill with technical build output - that is normal.", "Just wait for it to finish.")
        Write-Host ""
        & $PSCommandPath @Arguments
        $code = $LASTEXITCODE
        Write-Host ""
        if ($code -eq 0) {
            Write-Host "  Done! $FriendlyName finished successfully." -ForegroundColor Green
        } else {
            Show-GuiTroubleshooting -ExitCode $code
        }
        return $code
    }
    function Open-GuiOutputFolder {
        $path = Join-Path $ScriptDir $BuildDir
        if (-not (Test-Path -LiteralPath $path)) {
            Write-Host ""
            Write-Host "  Nothing has been built yet - that folder does not exist." -ForegroundColor Yellow
            return
        }
        Write-Host ""
        Write-Host "  Opening $path ..." -ForegroundColor Cyan
        Invoke-Item $path
    }
    function Invoke-GuiStripExisting {
        $strip = Join-Path $MingwBin "strip.exe"
        if (-not (Test-Path -LiteralPath $strip)) {
            Write-Host "  strip.exe not found - build once first so the toolchain is installed." -ForegroundColor Yellow
            return
        }
        $targets = @(
            (Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_native.exe")),
            (Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_server.exe"))
        )
        foreach ($t in $targets) {
            if (Test-Path -LiteralPath $t) {
                $before = [Math]::Round((Get-Item -LiteralPath $t).Length / 1MB, 1)
                & $strip --strip-debug --strip-unneeded $t
                $after = [Math]::Round((Get-Item -LiteralPath $t).Length / 1MB, 1)
                Write-Host "  $t : $before MB -> $after MB"
            }
        }
    }

    Show-GuiWelcome
    $running = $true
    while ($running) {
        $choice = (Show-GuiMenu).Trim()
        switch ($choice) {
            "1" {
                Write-GuiHeading "Build the game (Release)"
                if (Confirm-GuiAction "Start the build now?") {
                    Invoke-GuiBuild -Arguments @() -FriendlyName "Release build" | Out-Null
                }
            }
            "2" {
                Write-GuiHeading "Build and play"
                if (Confirm-GuiAction "Start the build now?") {
                    Invoke-GuiBuild -Arguments @("-Target", "Client", "-Run") -FriendlyName "Client build" | Out-Null
                }
            }
            "3" {
                Write-GuiHeading "Debug build"
                Write-GuiExplain @("Full DWARF symbols, no optimization. Slow to run, best for chasing crashes.")
                if (Confirm-GuiAction "Start the debug build now?") {
                    Invoke-GuiBuild -Arguments @("-BuildType", "Debug") -FriendlyName "Debug build" | Out-Null
                }
            }
            "4" {
                Write-GuiHeading "Profiling build"
                Write-GuiExplain @("Optimized (-O3) but keeps debug symbols, so it stays large. Good for profilers.")
                if (Confirm-GuiAction "Start the profiling build now?") {
                    Invoke-GuiBuild -Arguments @("-BuildType", "RelWithDebInfo") -FriendlyName "Profiling build" | Out-Null
                }
            }
            "5" {
                Write-GuiHeading "Start completely fresh"
                if (Confirm-GuiAction "Wipe the old build and start fresh?") {
                    Invoke-GuiBuild -Arguments @("-Clean") -FriendlyName "Clean rebuild" | Out-Null
                }
            }
            "6" {
                Write-GuiHeading "Build the multiplayer server"
                if (Confirm-GuiAction "Start the build now?") {
                    Invoke-GuiBuild -Arguments @("-Target", "Server") -FriendlyName "Server build" | Out-Null
                }
            }
            "7" {
                Write-GuiHeading "Build and run the test suite"
                if (Confirm-GuiAction "Start the build and tests now?") {
                    Invoke-GuiBuild -Arguments @("-RunTests") -FriendlyName "Build with tests" | Out-Null
                }
            }
            "8" {
                Write-GuiHeading "Strip debug symbols from built exe(s)"
                Invoke-GuiStripExisting
            }
            "9" {
                Write-GuiHeading "Exe sizes"
                Get-GuiExeSizes
            }
            "10" {
                Open-GuiOutputFolder
            }
            "11" {
                Write-GuiHeading "Clean build folder"
                if (Confirm-GuiAction "Wipe $BuildDir and the build lock file?") {
                    & $PSCommandPath -CleanOnly -BuildDir $BuildDir
                }
            }
            "12" {
                Write-GuiHeading "Format C++ source"
                & $PSCommandPath -Format
            }
            "13" {
                Show-GuiWelcome
            }
            "14" {
                $running = $false
            }
            default {
                Write-Host ""
                Write-Host "  Please type a number from 1 to 14." -ForegroundColor Yellow
            }
        }
    }
    Write-Host ""
    Write-Host "  Goodbye!" -ForegroundColor Cyan
    exit 0
}
# ===== end CLGUI =====

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

function Sync-Shaderpacks {
    param([string]$BuildDirName)

    if (-not (Test-Path -LiteralPath $ShaderpacksSource)) {
        Write-Error "Shaderpacks source directory not found: $ShaderpacksSource"
        return
    }

    $buildShaderpacks = Join-Path (Join-Path $ScriptDir $BuildDirName) "shaderpacks"
    Ensure-Directory -Path $buildShaderpacks
    $packs = Get-ChildItem -LiteralPath $ShaderpacksSource -Force
    foreach ($pack in $packs) {
        Copy-Item -LiteralPath $pack.FullName -Destination $buildShaderpacks -Recurse -Force
    }
    Write-Host "Shaderpacks: $buildShaderpacks"

    if ($env:APPDATA) {
        $runtimeRoot = Join-Path (Join-Path $env:APPDATA ".minecraft") "shaderpacks"
        Ensure-Directory -Path $runtimeRoot
        foreach ($pack in $packs) {
            Copy-Item -LiteralPath $pack.FullName -Destination $runtimeRoot -Recurse -Force
        }
        Write-Host "Shaderpacks deployed: $runtimeRoot"
    }
}

# resource::resourceRoot() (ResourceRoot.hpp) is always %APPDATA%/.minecraft/resources —
# that is where the running exe reads resource files from and where
# ResourceDownloadThread saves betacraft-downloaded sound/music. Mirror native/resources/
# there after every build via robocopy /MIR so only changed files are touched on
# rebuilds instead of a full recursive copy every time. Skip with -SkipResourceSync.
function Sync-Resources {
    if (-not (Test-Path -LiteralPath $ResourcesSource)) {
        Write-Host "Resources source directory not found: $ResourcesSource (skipping)"
        return
    }
    if (-not $env:APPDATA) {
        Write-Error "APPDATA is not set; cannot locate %APPDATA%\.minecraft\resources"
        return
    }

    $runtimeResources = Join-Path (Join-Path $env:APPDATA ".minecraft") "resources"
    $robocopy = Get-Command robocopy.exe -ErrorAction SilentlyContinue
    if ($robocopy) {
        & robocopy.exe $ResourcesSource $runtimeResources /MIR /MT:8 /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) {
            Write-Error "robocopy failed ($LASTEXITCODE) mirroring resources to $runtimeResources"
            return
        }
    } else {
        Ensure-Directory -Path $runtimeResources
        Copy-Item -Path (Join-Path $ResourcesSource "*") -Destination $runtimeResources -Recurse -Force
    }
    Write-Host "Resources: $runtimeResources"
}

# Was package-mods.ps1: builds runtime mod zips from native/mods/<mod_id>/ sources and
# deploys them to %APPDATA%\.minecraft\mods\. Runs by default after every build (see
# -SkipModPackaging); -ModId packages a single mod, -NoModDeploy skips the deploy copy.
function Invoke-PackageMods {
    param(
        [string]$BuildDirName,
        [string]$ModId = "",
        [bool]$Deploy = $true
    )

    $modsSource = Join-Path $ScriptDir "mods"
    if (-not (Test-Path -LiteralPath $modsSource)) {
        return
    }
    $modsOut = Join-Path $ScriptDir (Join-Path $BuildDirName "mods")
    $deployDir = Join-Path $env:APPDATA ".minecraft\mods"

    Ensure-Directory -Path $modsOut
    if ($Deploy) {
        Ensure-Directory -Path $deployDir
    }

    $modDirs = Get-ChildItem -LiteralPath $modsSource -Directory
    if ($ModId -ne "") {
        $modDirs = $modDirs | Where-Object { $_.Name -eq $ModId }
        if ($modDirs.Count -eq 0) {
            throw "Mod '$ModId' not found in $modsSource"
        }
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $packed = 0
    $deployed = 0

    foreach ($dir in $modDirs) {
        if ($dir.Name -eq "lib") {
            $libOut = Join-Path $modsOut "lib"
            Ensure-Directory -Path $libOut
            Copy-Item -Path (Join-Path $dir.FullName "*") -Destination $libOut -Recurse -Force
            Write-Host "  Packed shared library lib  ->  $libOut"
            $packed++

            if ($Deploy) {
                $libDest = Join-Path $deployDir "lib"
                Ensure-Directory -Path $libDest
                Copy-Item -Path (Join-Path $dir.FullName "*") -Destination $libDest -Recurse -Force
                Write-Host "  Deployed shared library lib  ->  $libDest"
                $deployed++
            }
            continue
        }
        $manifest = Join-Path $dir.FullName "mod.json"
        if (-not (Test-Path -LiteralPath $manifest)) {
            continue
        }

        $id = $dir.Name
        $zip = Join-Path $modsOut "$id.zip"
        if (Test-Path -LiteralPath $zip) {
            Remove-Item -LiteralPath $zip -Force
        }

        $files = Get-ChildItem -Recurse -File -LiteralPath $dir.FullName
        $stream = [System.IO.File]::Open($zip, [System.IO.FileMode]::Create)
        $archive = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create, $false, [System.Text.Encoding]::UTF8)
        try {
            foreach ($file in $files) {
                $entryName = $file.FullName.Substring($dir.FullName.Length + 1).Replace("\", "/")
                $entry = $archive.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
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

        $size = [Math]::Round((Get-Item -LiteralPath $zip).Length / 1KB, 1)
        Write-Host "  Packed $id  ->  $zip  ($size KB)"
        $packed++

        if ($Deploy) {
            $dest = Join-Path $deployDir "$id.zip"
            Copy-Item -LiteralPath $zip -Destination $dest -Force
            Write-Host "  Deployed $id  ->  $dest"
            $deployed++
        }
    }

    if ($packed -gt 0) {
        Write-Host "Packaged $packed mod(s)$(if ($Deploy) { ", deployed $deployed" })."
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
    # No -g here by default: Release is a small ship build. Pass -KeepDebugSymbols
    # (or use -BuildType RelWithDebInfo, which keeps -g via CMake's own defaults)
    # when an optimized binary with DWARF symbols is actually needed.
    $OmegaCxx = "-funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-semantic-interposition -fmerge-all-constants"
    if ($KeepDebugSymbols) {
        $OmegaCxx = "-g " + $OmegaCxx
    }
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
    Sync-Shaderpacks -BuildDirName $BuildDir
    if (-not $SkipResourceSync) {
        Sync-Resources
    }
    $StripExe = Join-Path $MingwBin "strip.exe"
    function Show-ExeInfo {
        param([string]$Label, [string]$Path)
        if (-not (Test-Path -LiteralPath $Path)) {
            return
        }
        if ($StripSymbols -and (Test-Path -LiteralPath $StripExe)) {
            Write-Host "Stripping symbols: $Path"
            & $StripExe --strip-debug --strip-unneeded $Path
        }
        $sizeMB = [Math]::Round((Get-Item -LiteralPath $Path).Length / 1MB, 1)
        Write-Host "${Label}: $Path ($sizeMB MB)"
    }

    if ($Target -eq "All" -or $Target -eq "Client") {
        Show-ExeInfo -Label "Client" -Path (Join-Path $BuildDir "minecraft_native.exe")
    }
    if ($Target -eq "All" -or $Target -eq "Server") {
        Show-ExeInfo -Label "Server" -Path (Join-Path $BuildDir "minecraft_server.exe")
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
    if (-not $SkipModPackaging -and $exitCode -eq 0) {
        try {
            Invoke-PackageMods -BuildDirName $BuildDir -ModId $ModId -Deploy (-not $NoModDeploy)
        } catch {
            Write-Error $_
            $exitCode = 1
        }
    }
}

Write-Host ("Finished in {0:N1}s (exit $exitCode)" -f $sw.Elapsed.TotalSeconds)

if ($Run -and $exitCode -eq 0) {
    $launchArgs = @()
    if ($Target -eq "Server") {
        $exe = Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_server.exe")
        if ($NoGui) { $launchArgs += "nogui" }
    } else {
        $exe = Join-Path $ScriptDir (Join-Path $BuildDir "minecraft_native.exe")
    }
    if (-not (Test-Path -LiteralPath $exe)) {
        Write-Error "Executable not found: $exe"
        exit 1
    }
    if ($RunArgs -and $RunArgs.Count -gt 0) {
        $launchArgs += $RunArgs
    }
    Write-Host "Launching $exe ..."
    if ($launchArgs.Count -gt 0) {
        & $exe @launchArgs
    } else {
        & $exe
    }
    $exitCode = $LASTEXITCODE
}

exit $exitCode

} finally {
    Release-BuildOmegaLock
}
