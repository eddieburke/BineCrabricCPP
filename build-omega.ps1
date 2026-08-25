param([string]$BuildDir="build-omega",[ValidateSet("Release","RelWithDebInfo","Debug")][string]$BuildType="Release",[int]$Jobs=0,[switch]$Clean,[switch]$Lto,[switch]$NoLto,[switch]$NoCcache,[switch]$NoNativeCpu,[switch]$RunTests,[string]$TestFilter="",[switch]$SkipModPackaging,[switch]$SkipResourceSync,[switch]$KeepDebugSymbols,[switch]$StripSymbols,[switch]$Log,[switch]$Gui,[switch]$Run,[switch]$NoGui,[switch]$CleanOnly,[switch]$Format,[string]$ModId="",[switch]$NoModDeploy,[ValidateSet("All","Client","Server")][string]$Target="All",[Parameter(ValueFromRemainingArguments=$true)][string[]]$RunArgs)
$ErrorActionPreference="Stop"
$ScriptDir=Split-Path -Parent $MyInvocation.MyCommand.Path
sl $ScriptDir
$ToolchainDir="$ScriptDir\toolchain"
$MingwRoot="$ToolchainDir\mingw64"
$DownloadsDir="$ToolchainDir\_downloads"
$ToolsDir="$ToolchainDir\tools"
$ManifestPath="$ToolchainDir\manifest.json"
$MingwBin="$MingwRoot\bin"
$GppExe="$MingwBin\g++.exe"
$CmakeExe="$MingwBin\cmake.exe"
$NinjaExe="$MingwBin\ninja.exe"
$CtestExe="$MingwBin\ctest.exe"
$DepsMarker="$ToolchainDir\.deps-installed"
$ShadersSource="$ScriptDir\shaders"
$ResourcesSource="$ScriptDir\resources"
$RelToolchainRoot="toolchain/mingw64"
$RelGpp="$RelToolchainRoot/bin/g++.exe"
$RelNinja="$RelToolchainRoot/bin/ninja.exe"
$BuildOmegaLockPath="$ScriptDir\.build-omega.lock"
function Fail{param([string]$Msg)Write-Host $Msg -ForegroundColor Red;exit 1}
if($CleanOnly){
$ct="$ScriptDir\$BuildDir"
Write-Host "Cleaning $BuildDir ..."
try{
if(Test-Path -Li $ct){Write-Host "Removing $ct";ri -Li $ct -R -Fo -EA Stop}
if(Test-Path -Li $BuildOmegaLockPath){Write-Host "Removing $BuildOmegaLockPath";ri -Li $BuildOmegaLockPath -Fo -EA Stop}
}catch{Write-Warning "Could not fully clean $BuildDir - it may be in use by another build. $_";exit 1}
Write-Host "Clean complete.";exit 0
}
if($Format){
$cf=$null
foreach($c in @("C:\Program Files\LLVM\bin\clang-format.exe","C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-format.exe")){if(Test-Path -Li $c){$cf=$c;break}}
if(!$cf){Fail "clang-format not found. Install LLVM or Visual Studio LLVM tools."}
$fl=@()
foreach($r in @("src","tests")){$rp="$ScriptDir\$r";if(!(Test-Path -Li $rp)){continue};foreach($e in @("*.cpp","*.hpp","*.h")){$fl+=gci -Path $rp -R -Fi $e -File}}
if($fl.Count -eq 0){Write-Host "No C++ files found to format.";exit 0}
Write-Host ("Formatting {0} files with {1}" -f $fl.Count,$cf)
$fa=0
foreach($f2 in $fl){&$cf -i $f2.FullName 2>&1|Out-Null;if($LASTEXITCODE -ne 0){$fa++;Write-Host ("clang-format failed: {0}" -f $f2.FullName)}}
if($fa -gt 0){Fail ("clang-format failed on {0} file(s)." -f $fa)}
Write-Host "clang-format complete.";exit 0
}
# ===== CLGUI =====
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
        Write-Host "  [14] Advanced options (build flags: LTO, symbols, mods, ...)"
        Write-Host "  [15] Exit"
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
        param([object]$Arguments, [string]$FriendlyName)
        $merged = @{}
        if ($null -ne $Arguments) {
            foreach ($k in $Arguments.Keys) { $merged[$k] = $Arguments[$k] }
        }
        $mergedBuildType = if ($merged.ContainsKey("BuildType")) { $merged["BuildType"] } else { "Release" }
        foreach ($k in $AdvancedOptions.Keys) {
            if ($merged.ContainsKey($k)) { continue }
            # LTO is only valid for Release builds; never inject it into Debug/Profiling builds.
            if ($k -eq "Lto" -and $mergedBuildType -ne "Release") { continue }
            $merged[$k] = $AdvancedOptions[$k]
        }
        Write-Host ""
        Write-Host "  Starting: $FriendlyName ..." -ForegroundColor Cyan
        Write-GuiExplain @("This window will fill with technical build output - that is normal.", "Just wait for it to finish.")
        Write-Host ""
        & $PSCommandPath @merged
        $code = $LASTEXITCODE
        Write-Host ""
        if ($code -eq 0) {
            Write-Host "  Done! $FriendlyName finished successfully." -ForegroundColor Green
        } else {
            Show-GuiTroubleshooting -ExitCode $code
        }
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
    $AdvancedOptions = @{
        Lto              = $false
        NoCcache         = $false
        NoNativeCpu      = $false
        KeepDebugSymbols = $false
        Log              = $false
        RunTests         = $false
        SkipResourceSync = $false
        SkipModPackaging = $false
    }
    function Get-GuiAdvancedLabels {
        return @{
            Lto              = "Link-time optimization (opt-in; Release only, may fail on MinGW GCC 15)"
            NoCcache         = "Disable ccache for this build directory"
            NoNativeCpu      = "Portable build (do not use -march=native)"
            KeepDebugSymbols = "Keep debug symbols in Release builds"
            Log              = "Save the full build log to build-omega-last.log"
            RunTests         = "Build the tests (add -TestFilter to run a subset instead of the whole suite)"
            SkipResourceSync = "Skip copying resources to %APPDATA%\\.minecraft"
            SkipModPackaging = "Skip packaging and deploying mods"
        }
    }
    function Show-GuiAdvancedOptions {
        $order = @("Lto", "NoCcache", "NoNativeCpu", "KeepDebugSymbols", "Log", "RunTests", "SkipResourceSync", "SkipModPackaging")
        $labels = Get-GuiAdvancedLabels
        $open = $true
        while ($open) {
            Write-GuiHeading "Advanced options (build flags)"
            Write-GuiExplain @("These settings apply to the build/run options in the main menu for this session.")
            Write-Host ""
            for ($i = 0; $i -lt $order.Count; $i++) {
                $state = if ($AdvancedOptions[$order[$i]]) { "On " } else { "Off" }
                Write-Host ("  [{0}] {1,-14} {2}   {3}" -f ($i + 1), $order[$i], $state, $labels[$order[$i]])
            }
            Write-Host ""
            Write-Host "  [A] Turn all on"
            Write-Host "  [N] Turn all off"
            Write-Host "  [B] Back to main menu"
            $answer = (Read-Host "  Pick a number, or A / N / B").Trim()
            if ($answer -match '^(b|back)$') { $open = $false; continue }
            if ($answer -match '^(a|all)$') { foreach ($k in $order) { $AdvancedOptions[$k] = $true }; continue }
            if ($answer -match '^(n|none)$') { foreach ($k in $order) { $AdvancedOptions[$k] = $false }; continue }
            if ($answer -match '^\d+$') {
                $idx = [int]$answer
                if ($idx -ge 1 -and $idx -le $order.Count) {
                    $key = $order[$idx - 1]
                    $AdvancedOptions[$key] = -not $AdvancedOptions[$key]
                    Write-Host ("  {0} is now {1}." -f $key, $(if ($AdvancedOptions[$key]) { "ON" } else { "off" })) -ForegroundColor Cyan
                    continue
                }
            }
            Write-Host "  Please enter a number from 1 to $($order.Count), A, N, or B." -ForegroundColor Yellow
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
                    Invoke-GuiBuild -Arguments @{} -FriendlyName "Release build"
                }
            }
            "2" {
                Write-GuiHeading "Build and play"
                if (Confirm-GuiAction "Start the build now?") {
                    Invoke-GuiBuild -Arguments @{"Target"="Client"; "Run"=$true} -FriendlyName "Client build"
                }
            }
            "3" {
                Write-GuiHeading "Debug build"
                Write-GuiExplain @("Full DWARF symbols, no optimization. Slow to run, best for chasing crashes.")
                if (Confirm-GuiAction "Start the debug build now?") {
                    Invoke-GuiBuild -Arguments @{"BuildType"="Debug"; "Target"="Client"} -FriendlyName "Debug build"
                }
            }
            "4" {
                Write-GuiHeading "Profiling build"
                Write-GuiExplain @("Optimized (-O3) but keeps debug symbols, so it stays large. Good for profilers.")
                if (Confirm-GuiAction "Start the profiling build now?") {
                    Invoke-GuiBuild -Arguments @{"BuildType"="RelWithDebInfo"} -FriendlyName "Profiling build"
                }
            }
            "5" {
                Write-GuiHeading "Start completely fresh"
                if (Confirm-GuiAction "Wipe the old build and start fresh?") {
                    Invoke-GuiBuild -Arguments @{"Clean"=$true} -FriendlyName "Clean rebuild"
                }
            }
            "6" {
                Write-GuiHeading "Build the multiplayer server"
                if (Confirm-GuiAction "Start the build now?") {
                    Invoke-GuiBuild -Arguments @{"Target"="Server"} -FriendlyName "Server build"
                }
            }
            "7" {
                Write-GuiHeading "Build and run the test suite"
                if (Confirm-GuiAction "Start the build and tests now?") {
                    Invoke-GuiBuild -Arguments @{"RunTests"=$true} -FriendlyName "Build with tests"
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
                Write-GuiHeading "Advanced options"
                Show-GuiAdvancedOptions
            }
            "15" {
                $running = $false
            }
            default {
                Write-Host ""
                Write-Host "  Please type a number from 1 to 15." -ForegroundColor Yellow
            }
        }
    }
    Write-Host ""
    Write-Host "  Goodbye!" -ForegroundColor Cyan
    exit 0
}
# ===== end CLGUI =====
function Get-ToolchainManifest{if(!(Test-Path -Li $ManifestPath)){Fail "Missing toolchain manifest: $ManifestPath"};return gc -Li $ManifestPath -Raw|ConvertFrom-Json}
function ED{param([string]$Pa)if(!(Test-Path -Li $Pa)){ni -ItemType Directory -Fo -Path $Pa|Out-Null}}
function Sync-Shaders{param([string]$BN)
if(!(Test-Path -Li $ShadersSource)){Write-Host "Shaders source directory not found: $ShadersSource" -ForegroundColor Red;return}
$bs="$ScriptDir\$BN\shaders";ED $bs;$pk=gci -Li $ShadersSource -Fo
foreach($p in $pk){cp -Li $p.FullName -De $bs -R -Fo}
Write-Host "Shaders: $bs"
# Replace the deployed packs but keep the per-pack "<pack>.txt" option files the
# client writes there; wiping the whole directory reverted every shader setting.
if($env:APPDATA){$rr="$env:APPDATA\.minecraft\shaders";if(Test-Path -Li $rr){gci -Li $rr -Fo|?{$_.Extension -ne ".txt"}|%{ri -Li $_.FullName -R -Fo -EA SilentlyContinue}};ED $rr;foreach($p in $pk){cp -Li $p.FullName -De $rr -R -Fo};Write-Host "Shaders deployed: $rr"}
}
function Sync-Resources{
if(!(Test-Path -Li $ResourcesSource)){Write-Host "Resources source directory not found: $ResourcesSource (skipping)";return}
if(!$env:APPDATA){Write-Host "APPDATA is not set; cannot locate %APPDATA%\.minecraft\resources" -ForegroundColor Red;return}
$rr="$env:APPDATA\.minecraft\resources"
$rb=gcm robocopy.exe -EA SilentlyContinue
if($rb){&robocopy.exe $ResourcesSource $rr /MIR /MT:8 /NFL /NDL /NJH /NJS /NP|Out-Null;if($LASTEXITCODE -ge 8){Write-Host "robocopy failed ($LASTEXITCODE) mirroring resources to $rr" -ForegroundColor Red;return}}
else{ED $rr;cp -Path "$ResourcesSource\*" -De $rr -R -Fo}
Write-Host "Resources: $rr"
}
function Invoke-PackageMods{param([string]$BN,[string]$MID="",[bool]$DP=$true)
$ms="$ScriptDir\mods";if(!(Test-Path -Li $ms)){return}
$mo="$ScriptDir\$BN\mods";$dd="$env:APPDATA\.minecraft\mods"
ED $mo
if($DP){if($MID -eq "" -and(Test-Path -Li $dd)){ri -Li $dd -R -Fo -EA SilentlyContinue};ED $dd}
$md=gci -Li $ms -Directory
if($MID -ne ""){$md=$md|?{$_.Name -eq $MID};if($md.Count -eq 0){throw "Mod '$MID' not found in $ms"}}
Add-Type -AN System.IO.Compression;Add-Type -AN System.IO.Compression.FileSystem
$pk=0;$deployedCount=0
foreach($d in $md){
if($d.Name -eq "lib"){$lo="$mo\lib";ED $lo;cp -Path "$($d.FullName)\*" -De $lo -R -Fo;Write-Host "  Packed shared library lib  ->  $lo";$pk++;if($DP){$ld="$dd\lib";ED $ld;cp -Path "$($d.FullName)\*" -De $ld -R -Fo;Write-Host "  Deployed shared library lib  ->  $ld";$deployedCount++};continue}
$mf="$($d.FullName)\mod.json";if(!(Test-Path -Li $mf)){continue}
$id=$d.Name;$zp="$mo\$id.zip";if(Test-Path -Li $zp){ri -Li $zp -Fo}
$fl=gci -R -File -Li $d.FullName
$st=[System.IO.File]::Open($zp,[System.IO.FileMode]::Create)
$ar=New-Object System.IO.Compression.ZipArchive($st,[System.IO.Compression.ZipArchiveMode]::Create,$false,[System.Text.Encoding]::UTF8)
try{foreach($f2 in $fl){$en=$f2.FullName.Substring($d.FullName.Length+1).Replace("\","/");$e=$ar.CreateEntry($en,[System.IO.Compression.CompressionLevel]::Optimal);$es=$e.Open();try{$by=[System.IO.File]::ReadAllBytes($f2.FullName);$es.Write($by,0,$by.Length)}finally{$es.Close()}}}finally{$ar.Dispose();$st.Dispose()}
$sz=[Math]::Round((gi -Li $zp).Length/1KB,1);Write-Host "  Packed $id  ->  $zp  ($sz KB)";$pk++
if($DP){$ds="$dd\$id.zip";cp -Li $zp -De $ds -Fo;Write-Host "  Deployed $id  ->  $ds";$deployedCount++}
}
if($pk -gt 0){Write-Host "Packaged $pk mod(s)$(if($DP){", deployed $deployedCount"})."}
}
function Download-File{param([string]$U,[string]$D)
ED(Split-Path -Parent $D)
if(Test-Path -Li $D){$ex=gi -Li $D;if($ex.Length -gt 0){return};ri -Li $D -Fo}
Write-Host "Downloading $(Split-Path -Leaf $D) ..."
$cu=gcm curl.exe -EA SilentlyContinue
if($cu){&curl.exe -L --fail --retry 3 --retry-delay 2 -o $D $U;if($LASTEXITCODE -ne 0){Fail "curl failed ($LASTEXITCODE) for $U"};return}
iwr -Uri $U -OutFile $D -UseBasicParsing
}
function Ensure-ZstdTool{$m=Get-ToolchainManifest;$ze="$ToolsDir\zstd.exe";if(Test-Path -Li $ze){return $ze};ED $ToolsDir;$ar="$DownloadsDir\$($m.zstd.archive)";Download-File -U $m.zstd.url -D $ar;$er="$ToolsDir\zstd-extract";if(Test-Path -Li $er){ri -Li $er -R -Fo};Expand-Archive -Li $ar -DestinationPath $er -Fo;$se="$er\$($m.zstd.exe_subpath)";if(!(Test-Path -Li $se)){Fail "zstd.exe not found in $($m.zstd.archive)"};cp -Li $se -De $ze -Fo;return $ze}
function Merge-Ucrt64IntoMingw64{param([string]$SD2,[string]$DR);$ur="$SD2\ucrt64";if(!(Test-Path -Li $ur)){Fail "MSYS2 package did not contain ucrt64/: $SD2"};foreach($s in @("bin","include","lib","share")){$fr="$ur\$s";if(!(Test-Path -Li $fr)){continue};$to="$DR\$s";ED $to;cp -Path "$fr\*" -De $to -R -Fo}}
function Install-MsysUcrtPackage{param([string]$U,[string]$PN,[string]$MD2,[string]$ZT);$fn=Split-Path -Leaf $U;$ap="$DownloadsDir\$fn";Download-File -U $U -D $ap;$tp="$DownloadsDir\$fn.tar";if(Test-Path -Li $tp){ri -Li $tp -Fo};&$ZT -d $ap -o $tp -f;if($LASTEXITCODE -ne 0){Fail "zstd failed extracting $PN"};$sg="$DownloadsDir\stage-$PN";if(Test-Path -Li $sg){ri -Li $sg -R -Fo};ED $sg;tar -xf $tp -C $sg;if($LASTEXITCODE -ne 0){Fail "tar failed extracting $PN"};Merge-Ucrt64IntoMingw64 -SD2 $sg -DR $MD2;ri -Li $sg -R -Fo}
function Test-ToolchainDepsPresent{param([string]$MD2);foreach($l in @("libz.a")){if(!(Test-Path -Li "$MD2\lib\$l")){return $false}};return $true}
function Ensure-ToolchainDeps{param([string]$MD2);if((Test-Path -Li $DepsMarker)-and(Test-ToolchainDepsPresent -MD2 $MD2)){return};Write-Host "Installing bundled zlib/Ogg/Vorbis dependencies into toolchain/mingw64 ...";$m=Get-ToolchainManifest;$zt=Ensure-ZstdTool;ED $MD2;foreach($p in $m.msys2_ucrt_packages){Write-Host "  -> $($p.name)";Install-MsysUcrtPackage -U $p.url -PN $p.name -MD2 $MD2 -ZT $zt};Set-Content -LiteralPath $DepsMarker -Value ("installed "+(Get-Date -Format "o")) -Encoding ASCII}
function Test-LocalToolchainPresent{return(Test-Path -Li $GppExe)-and(Test-Path -Li $CmakeExe)-and(Test-Path -Li $NinjaExe)}
function Ensure-BundledToolchain{if(Test-LocalToolchainPresent){Ensure-ToolchainDeps -MD2 $MingwRoot;Write-Host "Using downloaded toolchain: $RelToolchainRoot";return};$m=Get-ToolchainManifest;ED $DownloadsDir;$ap="$DownloadsDir\$($m.winlibs.archive)";Download-File -U $m.winlibs.url -D $ap;$sg="$ToolchainDir\_staging";if(Test-Path -Li $sg){ri -Li $sg -R -Fo};ED $sg;Write-Host "Extracting bundled GCC toolchain ...";Expand-Archive -Li $ap -DestinationPath $sg -Fo;$em="$sg\mingw64";if(!(Test-Path -Li $em)){$cd=gci -Li $sg -Directory|?{Test-Path "$($_.FullName)\bin\g++.exe"}|select -First 1;if($null -eq $cd){Fail "WinLibs archive did not contain mingw64/bin/g++.exe"};$em=$cd.FullName};if(Test-Path -Li $MingwRoot){ri -Li $MingwRoot -R -Fo};mi -Li $em -De $MingwRoot;ri -Li $sg -R -Fo -EA SilentlyContinue;if(!(Test-Path -Li $GppExe)){Fail "Bundled toolchain install failed: $GppExe"};Ensure-ToolchainDeps -MD2 $MingwRoot;Write-Host "Bundled toolchain ready: $MingwRoot"}
function Set-BundledToolchainEnvironment{param([string]$MBD);$fl=@();if($env:PATH){$fl=$env:PATH -split ';'|?{$_ -ne "" -and $_ -notmatch '(?i)msys64' -and $_ -notmatch '(?i)\\mingw64\\bin' -and $_ -notmatch '(?i)Microsoft Visual Studio'}};$env:PATH=($MBD+';'+($fl -join ';'));$env:CXX="$MBD\g++.exe";ri Env:\CC -EA SilentlyContinue}
$BuildOmegaLockStream=$null
function Remove-StaleBuildOmegaLockFile{param([string]$LPa)if(Test-Path -Li $LPa){ri -Li $LPa -Fo -EA SilentlyContinue}}
function Release-BuildOmegaLock{if($null -ne $script:BuildOmegaLockStream){try{$script:BuildOmegaLockStream.Close()}catch{};try{$script:BuildOmegaLockStream.Dispose()}catch{};$script:BuildOmegaLockStream=$null};if(Test-Path -Li $script:BuildOmegaLockPath){ri -Li $script:BuildOmegaLockPath -Fo -EA SilentlyContinue}}
function Get-ExternalBuildProcesses{param([string]$BN,[string]$SDP);$ba=("$SDP\$BN").Replace("\","/");$bl=$BN.Replace("\","/");$mt=@();gcim Win32_Process -EA SilentlyContinue|?{$_.Name -eq "cmake.exe" -or $_.Name -eq "ninja.exe"}|%{$cm=$_.CommandLine;if(!$cm){return};$cn=$cm.Replace("\","/");if($cn -like "*$ba*" -or $cn -like "*--build*$bl*"){$mt+=$_}};return $mt}
function Assert-BuildDirAvailable{param([string]$BN,[string]$SDP);$pr=Get-ExternalBuildProcesses -BN $BN -SDP $SDP;if($pr.Count -eq 0){return};$dt=($pr|%{"  PID $($_.ProcessId): $($_.Name)"}) -join [Environment]::NewLine;Fail "Another cmake/ninja build is already using '$BN'.`nWait for it to finish, or stop those processes, then retry.`n$dt"}
function Remove-StaleNinjaRestatFile{param([string]$BP);$rs="$BP\.ninja_log.restat";if(Test-Path -Li $rs){ri -Li $rs -Fo -EA SilentlyContinue}}
function Stop-BuiltExecutable{param([string]$Pth)
$fp=[System.IO.Path]::GetFullPath($Pth)
$ps=gcim Win32_Process -EA SilentlyContinue|?{$_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$fp,[System.StringComparison]::OrdinalIgnoreCase)}
foreach($p in $ps){Write-Host "Stopping running build output: $($p.Name) (PID $($p.ProcessId))";Stop-Process -Id $p.ProcessId -Force -EA Stop}
}
function Prepare-Relink{param([string[]]$Paths)
$result=@()
foreach($p in $Paths){
$full=[System.IO.Path]::GetFullPath($p);$backup="$full.build-omega-backup"
if((Test-Path -Li $backup)-and!(Test-Path -Li $full)){mi -Li $backup -De $full -Fo}
elseif(Test-Path -Li $backup){ri -Li $backup -Fo}
if(Test-Path -Li $full){Stop-BuiltExecutable -Pth $full;mi -Li $full -De $backup -Fo;$result+=$backup}
}
return $result
}
function Complete-Relink{param([string[]]$Backups,[bool]$Succeeded)
foreach($backup in $Backups){
$output=$backup.Substring(0,$backup.Length-19)
if($Succeeded){if(Test-Path -Li $backup){ri -Li $backup -Fo}}
elseif(!(Test-Path -Li $output)-and(Test-Path -Li $backup)){mi -Li $backup -De $output -Fo;Write-Host "Restored previous executable: $output" -ForegroundColor Yellow}
}
}
function Show-BuildFailureSummary{param([string]$LogPath,[int]$Code)
Write-Host ""
Write-Host "Build failed (exit $Code)." -ForegroundColor Red
if(Test-Path -Li $LogPath){
$hits=Select-String -Path $LogPath -Pattern '(^|[\s:])(fatal error|error:|undefined reference|cannot (open|find)|collect2\.exe:|ninja: build stopped|FAILED:)' -CaseSensitive:$false|%{$_.Line.Trim()}|?{$_}|select -Unique -First 8
if($hits){Write-Host "Relevant diagnostics:" -ForegroundColor Yellow;foreach($hit in $hits){Write-Host "  $hit" -ForegroundColor Red}}
Write-Host "Full build log: $LogPath" -ForegroundColor Yellow
}
}
function Normalize-ToolPath{param([string]$Pa)if(!$Pa){return ""};return $Pa.Replace("\","/").ToLowerInvariant()}
function Test-NeedsConfigure{param([string]$BP,[string]$EC,[string]$ETR,[string]$EBT,[bool]$UL,[bool]$UCC,[string]$ECFR="")
$cp="$BP\CMakeCache.txt"
if(!(Test-Path -Li $cp)){return $true}
if(!(Test-Path -Li "$BP\build.ninja")){return $true}
$gn=$null;$co=$null;$tr=$null;$bt=$null;$ip=$null;$ucc=$null;$cfr=$null;$cfrd=$null
foreach($ln in gc -Li $cp){
$v=$ln.Substring($ln.IndexOf("=")+1)
if($ln -like "CMAKE_GENERATOR:INTERNAL=*"){$gn=$v}
elseif($ln -like "CMAKE_CXX_COMPILER:FILEPATH=*" -or $ln -like "CMAKE_CXX_COMPILER:UNINITIALIZED=*" -or $ln -like "CMAKE_CXX_COMPILER:STRING=*"){$co=$v}
elseif($ln -like "MINECRAFT_TOOLCHAIN_ROOT:PATH=*" -or $ln -like "MINECRAFT_TOOLCHAIN_ROOT:UNINITIALIZED=*"){$tr=$v}
elseif($ln -like "CMAKE_BUILD_TYPE:STRING=*"){$bt=$v}
elseif($ln -like "CMAKE_INTERPROCEDURAL_OPTIMIZATION:BOOL=*"){$ip=$v}
elseif($ln -like "MINECRAFT_USE_CCACHE:BOOL=*"){$ucc=$v}
elseif($ln -like "CMAKE_CXX_FLAGS_RELEASE:STRING=*"){$cfr=$v.Trim()}
elseif($ln -like "CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=*"){$cfrd=$v.Trim()}
}
if($gn -ne "Ninja"){return $true}
if((Normalize-ToolPath $co) -ne (Normalize-ToolPath $EC)){return $true}
if($tr -and((Normalize-ToolPath $tr) -ne (Normalize-ToolPath $ETR))){return $true}
if($bt -ne $EBT){return $true}
if(($ip -eq "ON") -ne $UL){return $true}
$expectedCcache=if($UCC){"ON"}else{"OFF"}
if($ucc -ne $expectedCcache){return $true}
if($ECFR -ne ""){$en=($ECFR -replace '\s+',' ').Trim();$cn=if($EBT -eq "Release"){$cfr}else{$cfrd};$cn=if($cn){($cn -replace '\s+',' ').Trim()}else{""};if($cn -ne $en){return $true}}
return $false
}
Ensure-BundledToolchain
Set-BundledToolchainEnvironment -MBD $MingwBin
if(!(Test-Path -Li $CmakeExe)){Fail "Bundled cmake not found at $CmakeExe"}
if(!(Test-Path -Li $NinjaExe)){Fail "Bundled ninja not found at $NinjaExe"}
Remove-StaleBuildOmegaLockFile -LPa $BuildOmegaLockPath
try {
if(Test-Path -Li $BuildOmegaLockPath){Remove-StaleBuildOmegaLockFile -LPa $BuildOmegaLockPath}
try{$BuildOmegaLockStream=[System.IO.File]::Open($BuildOmegaLockPath,[System.IO.FileMode]::Create,[System.IO.FileAccess]::ReadWrite,[System.IO.FileShare]::None);$pb=[System.Text.Encoding]::ASCII.GetBytes("$PID`n");$BuildOmegaLockStream.Write($pb,0,$pb.Length);$BuildOmegaLockStream.Flush()}catch{Fail "Already in use"}
if($Jobs -le 0){$Jobs=[Math]::Min(4,[Math]::Max(1,[Environment]::ProcessorCount-2))}
if($Jobs -gt 4){Write-Host "Limiting parallel jobs to 4 to avoid MinGW compiler out-of-memory failures." -ForegroundColor Yellow;$Jobs=4}
if($Clean -and(Test-Path $BuildDir)){Assert-BuildDirAvailable -BN $BuildDir -SDP $ScriptDir;Write-Host "Removing $BuildDir ...";ri -R -Fo $BuildDir;$cc="$MingwBin\ccache.exe";if(Test-Path -Li $cc){Write-Host "Clearing ccache (stale LTO objects) ...";&$cc -C}}
Assert-BuildDirAvailable -BN $BuildDir -SDP $ScriptDir
Write-Host "Toolchain: $RelToolchainRoot"
Write-Host "Compiler:  $RelGpp"
$CmakeArgs=@("-S",".","-B",$BuildDir,"-G","Ninja","-DCMAKE_MAKE_PROGRAM=$NinjaExe","-DCMAKE_CXX_COMPILER=$GppExe","-DCMAKE_BUILD_TYPE=$BuildType","-DMINECRAFT_TOOLCHAIN_ROOT=$RelToolchainRoot")
$UseLto=$Lto -and(-not $NoLto)
$UseCcache=-not $NoCcache
$CmakeArgs+="-DMINECRAFT_USE_CCACHE=$(if($UseCcache){'ON'}else{'OFF'})"
if($NoCcache){$CmakeArgs+="-DCMAKE_CXX_COMPILER_LAUNCHER=";$env:CCACHE_DISABLE="1"}
if($Lto -and $NoLto){Fail "-Lto and -NoLto cannot be used together."}
if($UseLto -and $BuildType -ne "Release"){Fail "-Lto is only supported with -BuildType Release."}
if($UseLto){Write-Host "LTO enabled (opt-in; may fail to link on MinGW GCC 15)";$CmakeArgs+="-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON";$CmakeArgs+="-DCMAKE_CXX_COMPILER_LAUNCHER=";$env:CCACHE_DISABLE="1"}
$OmegaCxx=""
if($BuildType -eq "Release"){
# Setting CMAKE_CXX_FLAGS_RELEASE replaces CMake's default "-O3 -DNDEBUG" wholesale.
# Omitting an -O level here does not fall back to CMake's -O3, it falls back to the
# compiler default of -O0, so the level has to be spelled out.
$OC="-O3 -DNDEBUG -funroll-loops -fomit-frame-pointer -ffunction-sections -fdata-sections -fno-semantic-interposition -fmerge-all-constants -fno-ident"
# No -g, and -s discards the symbol table and every debug section at link time. The
# crash reporter only ever prints raw addresses and module-relative offsets, so it
# reads the same either way. -KeepDebugSymbols puts both back.
$OL="-Wl,--gc-sections -s"
if($KeepDebugSymbols){$OC="-g "+$OC;$OL="-Wl,--gc-sections"}
# --gc-sections is what fails to link under LTO on MinGW GCC 15, not the strip.
if($UseLto){$OC=$OC+" -flto-partition=one";if($KeepDebugSymbols){$OL=""}else{$OL="-s"}}
if(!$NoNativeCpu){$OC="-march=native -mtune=native -mprefer-vector-width=128 "+$OC}
$OmegaCxx=$OC
$CmakeArgs+="-DCMAKE_CXX_FLAGS_RELEASE=$OC"
if($OL -ne ""){$CmakeArgs+="-DCMAKE_EXE_LINKER_FLAGS_RELEASE=$OL"}
}elseif($BuildType -eq "RelWithDebInfo"){
$OC="-O2 -g"
$OmegaCxx=$OC
$CmakeArgs+="-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=$OC"
}
$NeedsConfigure=Test-NeedsConfigure -BP "$ScriptDir\$BuildDir" -EC $GppExe -ETR $RelToolchainRoot -EBT $BuildType -UL $UseLto -UCC $UseCcache -ECFR $OmegaCxx
if($NeedsConfigure){
    if($BuildType -eq "Debug"){Write-Host "build-omega Debug build started";Write-Host "WARNING: This will take a very long time (1.3h+)"}
    Assert-BuildDirAvailable -BN $BuildDir -SDP $ScriptDir
    Remove-StaleNinjaRestatFile -BP "$ScriptDir\$BuildDir"
    if($BuildType -eq "Release"){Write-Host "Configuring $BuildDir (Ninja + bundled GCC, Release, omega flags) ..."}else{Write-Host "Configuring $BuildDir (Ninja + bundled GCC, $BuildType) ..."}
    &$CmakeExe @CmakeArgs
    if($LASTEXITCODE -ne 0){Write-Host "Configuration failed (exit $LASTEXITCODE). See the CMake error above." -ForegroundColor Red;exit $LASTEXITCODE}
}
Assert-BuildDirAvailable -BN $BuildDir -SDP $ScriptDir
$buildTargets=@("minecraft_native","minecraft_server","minecraft_installer")
$buildLabel="minecraft_native + minecraft_server + minecraft_installer"
if($Target -eq "Client"){$buildTargets=@("minecraft_native");$buildLabel="minecraft_native (client)"}
elseif($Target -eq "Server"){$buildTargets=@("minecraft_server");$buildLabel="minecraft_server"}
if($BuildType -eq "Release"){Write-Host "Omega build: $buildLabel (-j $Jobs) ..."}else{Write-Host "$BuildType build: $buildLabel (-j $Jobs) ..."}
$relinkPaths=$buildTargets|%{"$ScriptDir\$BuildDir\$_.exe"}
$relinkBackups=Prepare-Relink -Paths $relinkPaths
$buildLog="$ScriptDir\$BuildDir\build-omega-last.log"
$sw=[System.Diagnostics.Stopwatch]::StartNew()
if($Log){&$CmakeExe --build $BuildDir --target @buildTargets -j $Jobs 2>&1|Tee-Object -FilePath $buildLog}else{&$CmakeExe --build $BuildDir --target @buildTargets -j $Jobs}
$exitCode=$LASTEXITCODE
$sw.Stop()
Complete-Relink -Backups $relinkBackups -Succeeded:($exitCode -eq 0)
if($exitCode -ne 0){if($Log){Show-BuildFailureSummary -LogPath $buildLog -Code $exitCode}else{Write-Host "Build failed (exit $exitCode). Compiler and linker diagnostics are shown above. Use -Log to save them." -ForegroundColor Red}}
if($exitCode -eq 0){
Sync-Shaders -BN $BuildDir
if(!$SkipResourceSync){Sync-Resources}
$StripExe="$MingwBin\strip.exe"
function Show-ExeInfo{param([string]$Label,[string]$Pth)
if(!(Test-Path -Li $Pth)){return}
if($StripSymbols -and(Test-Path -Li $StripExe)){Write-Host "Stripping symbols: $Pth";&$StripExe --strip-debug --strip-unneeded $Pth}
$s=[Math]::Round((gi -Li $Pth).Length/1MB,1)
Write-Host "${Label}: $Pth ($s MB)"
}
if($Target -eq "All" -or $Target -eq "Client"){Show-ExeInfo -Label "Client" -Pth "$BuildDir\minecraft_native.exe"}
if($Target -eq "All" -or $Target -eq "Server"){Show-ExeInfo -Label "Server" -Pth "$BuildDir\minecraft_server.exe"}
Show-ExeInfo -Label "Installer" -Pth "$BuildDir\minecraft_installer.exe"
if($RunTests){
$tt=@()
if($Target -eq "All" -or $Target -eq "Client" -or $Target -eq "Server"){$tt+="minecraft_omega_tests"}
if($tt.Count -gt 0){
Write-Host "Building $($tt -join ', ') (-j $Jobs) ..."
&$CmakeExe --build $BuildDir --target @tt -j $Jobs
if($LASTEXITCODE -ne 0){$exitCode=$LASTEXITCODE}
else{
$ba="$ScriptDir\$BuildDir"
if($TestFilter -ne ""){
# Targeted run: drive the gtest binary directly. ctest re-launches one process per
# test case and holds the build lock for the whole suite, so a full pass blocks the
# next rebuild for 10+ minutes and restores the previous binary on the way out.
$te="$ScriptDir\$BuildDir\minecraft_omega_tests.exe"
if(!(Test-Path -Li $te)){Write-Host "Test binary not found at $te" -ForegroundColor Red;$exitCode=1}
else{
Write-Host "Running tests matching '$TestFilter' ..."
&$te --gtest_filter=$TestFilter --gtest_brief=1
if($LASTEXITCODE -ne 0){$exitCode=$LASTEXITCODE}
}
}else{
Write-Host "Running ctest in $BuildDir ..."
Push-Location $ba
try{
if(!(Test-Path -Li $CtestExe)){Write-Host "Bundled ctest not found at $CtestExe" -ForegroundColor Red;$exitCode=1}
else{&$CtestExe --output-on-failure;if($LASTEXITCODE -ne 0){$exitCode=$LASTEXITCODE}}
}finally{Pop-Location}
}
}
}
}
if(!$SkipModPackaging){
    try{Invoke-PackageMods -BN $BuildDir -MID $ModId -DP:$(-not $NoModDeploy)}catch{Write-Host $_ -ForegroundColor Red;$exitCode=1}
}
if($Gui -and $BuildType -eq "Release" -and !$Run){
$se=@()
if($Target -eq "All" -or $Target -eq "Client"){$se+="minecraft_native.exe"}
if($Target -eq "All" -or $Target -eq "Server"){$se+="minecraft_server.exe"}
if($Target -eq "All"){$se+="minecraft_installer.exe"}
if($se.Count -gt 0){
try{
Add-Type -AN System.Windows.Forms
$null=[System.Windows.Forms.Application]::EnableVisualStyles()
$f2=New-Object System.Windows.Forms.FolderBrowserDialog
$f2.Description="Select output folder for built binaries"
$f2.ShowNewFolderButton=$true
if($f2.ShowDialog() -eq "OK"){
foreach($x in $se){$s="$BuildDir\$x";if(Test-Path -Li $s){cp -Li $s -De "$($f2.SelectedPath)\$x" -Fo;Write-Host "Copied $x -> $($f2.SelectedPath)"}}
Start-Process explorer.exe -ArgumentList $f2.SelectedPath
}
}catch{Write-Warning "Copy dialog failed: $_"}
}
}
}
Write-Host ("Finished in {0:N1}s (exit $exitCode)" -f $sw.Elapsed.TotalSeconds)
if($Run -and $exitCode -eq 0){
$la=@()
if($Target -eq "Server"){$ex="$ScriptDir\$BuildDir\minecraft_server.exe";if($NoGui){$la+="nogui"}}
else{$ex="$ScriptDir\$BuildDir\minecraft_native.exe"}
if(!(Test-Path -Li $ex)){Fail "Executable not found: $ex"}
if($RunArgs -and $RunArgs.Count -gt 0){$la+=$RunArgs}
Write-Host "Launching $ex ..."
if($la.Count -gt 0){&$ex @la}else{&$ex}
$exitCode=$LASTEXITCODE
}
exit $exitCode
} finally {
Release-BuildOmegaLock
}
