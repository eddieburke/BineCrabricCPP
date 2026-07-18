param()

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$BuildOmega = Join-Path $ScriptDir "build-omega.ps1"
$BuildClient = Join-Path $ScriptDir "build-client.ps1"
$BuildServer = Join-Path $ScriptDir "build-server.ps1"
$BuildDirName = "build-omega"
$GppExe = Join-Path $ScriptDir "toolchain\mingw64\bin\g++.exe"

function Write-Rule {
    Write-Host ("-" * 70) -ForegroundColor DarkGray
}

function Write-Heading {
    param([string]$Text)
    Write-Host ""
    Write-Rule
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Rule
}

function Write-Explain {
    param([string[]]$Lines)
    foreach ($line in $Lines) {
        Write-Host "  $line" -ForegroundColor Gray
    }
}

function Pause-ForUser {
    param([string]$Prompt = "Press Enter to continue...")
    Read-Host $Prompt | Out-Null
}

function Confirm-Action {
    param([string]$Prompt)
    Write-Host ""
    $answer = Read-Host "$Prompt [Enter = Yes, type N = No]"
    return -not ($answer -match '^(n|no)$')
}

function Test-ToolchainInstalled {
    return Test-Path -LiteralPath $GppExe
}

function Test-InternetReachable {
    try {
        $result = Test-Connection -ComputerName "github.com" -Count 1 -Quiet -ErrorAction Stop
        return $result
    } catch {
        return $true
    }
}

function Get-FreeSpaceGB {
    try {
        $drive = (Get-Item -LiteralPath $ScriptDir).PSDrive
        return [Math]::Round($drive.Free / 1GB, 1)
    } catch {
        return $null
    }
}

function Show-Welcome {
    Write-Heading "Minecraft (Beta 1.7.3 native port) - Build Assistant"
    Write-Explain @(
        "This tool turns the game's source code into a program you can run."
        "That process is called 'building' or 'compiling'."
        ""
        "You do not need to know how to program to use this. Just pick a"
        "number from the menu and this tool will do the rest."
        ""
        "The very first time you build, this tool needs a 'compiler toolkit'"
        "(the software that turns source code into a .exe file). If it is not"
        "already on this computer, it will be downloaded automatically into"
        "the 'toolchain' folder next to this script - you do not need to"
        "install Visual Studio, MinGW, CMake, or anything else yourself."
    )

    if (Test-ToolchainInstalled) {
        Write-Host ""
        Write-Host "  Status: build toolkit is already installed on this computer." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "  Status: build toolkit NOT installed yet." -ForegroundColor Yellow
        Write-Explain @(
            "The first build will download it (roughly 150-350 MB, a few"
            "minutes depending on your internet speed). This only happens once."
        )
        $freeGB = Get-FreeSpaceGB
        if ($freeGB -ne $null) {
            Write-Explain @("Free disk space available: $freeGB GB (about 3 GB recommended).")
        }
        if (-not (Test-InternetReachable)) {
            Write-Host ""
            Write-Host "  Warning: could not reach the internet just now." -ForegroundColor Yellow
            Write-Explain @(
                "The first build needs an internet connection to download the"
                "toolkit. If you are offline, connect to the internet and try again."
            )
        }
    }
}

function Show-Menu {
    Write-Host ""
    Write-Rule
    Write-Host "  What would you like to do?" -ForegroundColor Cyan
    Write-Rule
    Write-Host "  [1] Build the game (recommended - makes the client and server)"
    Write-Host "  [2] Build the game AND start playing right away"
    Write-Host "  [3] Start completely fresh (fixes weird build problems, slower)"
    Write-Host "  [4] Build only the multiplayer server (for hosting)"
    Write-Host "  [5] Build and double-check everything works (runs the test suite)"
    Write-Host "  [6] Open the folder with the finished game"
    Write-Host "  [7] Explain all this again"
    Write-Host "  [8] Exit"
    Write-Host ""
    return Read-Host "Type a number and press Enter"
}

function Show-Troubleshooting {
    param([int]$ExitCode)
    Write-Host ""
    Write-Host "  Something went wrong (exit code $ExitCode)." -ForegroundColor Red
    Write-Explain @(
        "A few common causes, in order of likelihood:"
        ""
        "  - No internet connection during the first build (the toolkit"
        "    download failed). Check your connection and try again."
        "  - Antivirus or a firewall blocked the download or the compiler."
        "    You may need to allow this 'native' folder in your antivirus."
        "  - The game or a previous build is still running. Close"
        "    minecraft_native.exe / minecraft_server.exe and try again."
        "  - Not enough free disk space (a build needs a few GB free)."
        ""
        "You can simply try the same menu option again - builds pick up"
        "where they left off instead of starting over from nothing."
        "If it keeps failing, option [3] (start completely fresh) clears"
        "out any half-finished build and tries again from a clean state."
    )
}

function Invoke-BuildScript {
    param(
        [string]$Path,
        [string[]]$Arguments,
        [string]$FriendlyName
    )

    Write-Host ""
    Write-Host "  Starting: $FriendlyName ..." -ForegroundColor Cyan
    Write-Explain @("This window will fill with technical build output - that is normal.", "Just wait for it to finish.")
    Write-Host ""

    & $Path @Arguments
    $code = $LASTEXITCODE

    Write-Host ""
    if ($code -eq 0) {
        Write-Host "  Done! $FriendlyName finished successfully." -ForegroundColor Green
    } else {
        Show-Troubleshooting -ExitCode $code
    }
    return $code
}

function Open-OutputFolder {
    $path = Join-Path $ScriptDir $BuildDirName
    if (-not (Test-Path -LiteralPath $path)) {
        Write-Host ""
        Write-Host "  Nothing has been built yet - that folder does not exist." -ForegroundColor Yellow
        Write-Explain @("Pick option [1] first to build the game.")
        return
    }
    Write-Host ""
    Write-Host "  Opening $path ..." -ForegroundColor Cyan
    Write-Explain @(
        "Look for 'minecraft_native.exe' (the game) or 'minecraft_server.exe'"
        "(the multiplayer server) in that folder."
    )
    Invoke-Item $path
}

Show-Welcome

$running = $true
while ($running) {
    $choice = Show-Menu

    switch ($choice.Trim()) {
        "1" {
            Write-Heading "Build the game"
            Write-Explain @(
                "This builds both the game client and the multiplayer server."
                "Depending on your computer and whether the toolkit is already"
                "installed, this can take anywhere from under a minute to"
                "20+ minutes the very first time."
            )
            if (Confirm-Action "Start the build now?") {
                Invoke-BuildScript -Path $BuildOmega -Arguments @() -FriendlyName "Full build" | Out-Null
            }
        }
        "2" {
            Write-Heading "Build and play"
            Write-Explain @("This builds the game client, then launches it automatically when done.")
            if (Confirm-Action "Start the build now?") {
                Invoke-BuildScript -Path $BuildClient -Arguments @("-Run") -FriendlyName "Build and launch" | Out-Null
            }
        }
        "3" {
            Write-Heading "Start completely fresh"
            Write-Explain @(
                "This deletes the previous build folder and rebuilds everything"
                "from scratch. Use this if a normal build keeps failing in a way"
                "that doesn't make sense - it usually fixes it, but takes longer."
            )
            if (Confirm-Action "Wipe the old build and start fresh?") {
                Invoke-BuildScript -Path $BuildOmega -Arguments @("-Clean") -FriendlyName "Clean rebuild" | Out-Null
            }
        }
        "4" {
            Write-Heading "Build the multiplayer server"
            Write-Explain @(
                "This builds only 'minecraft_server.exe', the program other"
                "players connect to when you host a multiplayer game."
            )
            if (Confirm-Action "Start the build now?") {
                Invoke-BuildScript -Path $BuildServer -Arguments @() -FriendlyName "Server build" | Out-Null
            }
        }
        "5" {
            Write-Heading "Build and run the test suite"
            Write-Explain @(
                "This builds the game and then runs its automated tests, which"
                "check that core game logic still behaves correctly. Mainly"
                "useful if you (or someone helping you) changed the source code."
            )
            if (Confirm-Action "Start the build and tests now?") {
                Invoke-BuildScript -Path $BuildOmega -Arguments @("-RunTests") -FriendlyName "Build with tests" | Out-Null
            }
        }
        "6" {
            Open-OutputFolder
        }
        "7" {
            Show-Welcome
        }
        "8" {
            $running = $false
        }
        default {
            Write-Host ""
            Write-Host "  Please type a number from 1 to 8." -ForegroundColor Yellow
        }
    }
}

Write-Host ""
Write-Host "  Goodbye!" -ForegroundColor Cyan
Pause-ForUser -Prompt "Press Enter to close this window"
