# Hotspot profile of a running client. Launches the game, lets it load into the
# world, attaches VTune by PID, and collects until YOU close the game.
#
# There is deliberately NO synthetic input here. The old vtune-renderscale3.ps1 /
# vtune-gameplay-trace.ps1 drove the player with SendKeys plus a SetForegroundWindow
# call every 240ms, which stole focus from whatever the user was doing and typed "w"
# and space into it. The window is yours for the whole collection -- VTune is
# attached by PID and does not care who is at the keyboard.
param(
    [string]$World       = "streamstress",
    [int]   $Seed        = 31337,
    [int]   $LoadSeconds = 45,
    [string]$Name        = "vtune-rs5",
    [int]   $DataLimitMB = 20000,
    [switch]$DebugHud,
    [switch]$NoReport
)
$ErrorActionPreference = "Stop"

$Root  = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Exe   = "$Root\build-vtune\minecraft_native.exe"
$Vtune = "C:\Program Files (x86)\Intel\oneAPI\vtune\2026.4\bin64\vtune.exe"

if (!(Test-Path -LiteralPath $Exe))   { Write-Host "Missing $Exe -- build it with: .\build-omega.ps1 -BuildDir build-vtune -BuildType RelWithDebInfo -Target Client -Vtune" -ForegroundColor Red; exit 1 }
if (!(Test-Path -LiteralPath $Vtune)) { Write-Host "Missing $Vtune" -ForegroundColor Red; exit 1 }

# renderScale is the render-distance multiplier: blocks = (256 >> viewDistance) * renderScale
# (option/RenderSettings.cpp). Report what the run will actually use rather than assume it.
$optionsPath = "$env:APPDATA\.minecraft\options.txt"
if (Test-Path -LiteralPath $optionsPath) {
    $opts = @{}
    foreach ($line in Get-Content -LiteralPath $optionsPath) { $kv = $line -split ":", 2; if ($kv.Count -eq 2) { $opts[$kv[0]] = $kv[1] } }
    $vd = [int]$opts["viewDistance"]; $rs = [double]$opts["renderScale"]
    $blocks = (256 -shr $vd) * $rs
    Write-Host ("Render distance: renderScale={0}, viewDistance={1} -> {2} blocks ({3} chunks)" -f $rs, $vd, $blocks, [math]::Floor(($blocks + 8) / 16))
}

# vtune.exe chokes on the space in "New project 2", so hand it the 8.3 path.
$fso         = New-Object -ComObject Scripting.FileSystemObject
$shortRoot   = $fso.GetFolder($Root).ShortPath
$resultDir   = "$Root\$Name"
$shortResult = "$shortRoot\$Name"
if (Test-Path -LiteralPath $resultDir) { Remove-Item -Recurse -Force $resultDir }

$gameArgs = @("--world", $World, "--seed", "$Seed")
if ($DebugHud) { $gameArgs += "--debug-hud" }

$proc = Start-Process -FilePath $Exe -ArgumentList $gameArgs -PassThru
Write-Host "Launched PID $($proc.Id) -- loading for ${LoadSeconds}s"

Start-Sleep -Seconds $LoadSeconds
$proc.Refresh()
if ($proc.HasExited) { Write-Host "Game exited early during load, exit code $($proc.ExitCode)" -ForegroundColor Red; exit 1 }

# No -duration: collection runs open-ended and is stopped when the game exits.
# -data-limit caps the result dir so a long session cannot quietly eat the disk.
$collectArgs = "-collect hotspots -knob sampling-mode=sw -knob enable-stack-collection=true " +
               "-data-limit=$DataLimitMB -result-dir $shortResult -target-pid $($proc.Id)"
$vtuneProc = Start-Process -FilePath $Vtune -ArgumentList $collectArgs -PassThru `
    -RedirectStandardOutput "$Root\$Name.log" -RedirectStandardError "$Root\$Name.err.log"
Write-Host "VTune attached (PID $($vtuneProc.Id)). Collecting until you close the game. Play normally."

$proc.WaitForExit()
Write-Host "Game closed. Stopping collection ..."

# VTune normally finalizes on its own when the attached target dies; -command stop is
# the belt-and-braces path for when it does not notice.
$vtuneProc.Refresh()
if (-not $vtuneProc.HasExited) {
    if (-not $vtuneProc.WaitForExit(20000)) {
        Start-Process -FilePath $Vtune -ArgumentList "-command stop -result-dir $shortResult" -Wait -NoNewWindow `
            -RedirectStandardOutput "$Root\$Name-stop.log" -RedirectStandardError "$Root\$Name-stop.err.log"
        $vtuneProc.WaitForExit()
    }
}
Write-Host "vtune exited with code $($vtuneProc.ExitCode) -- finalizing reports"

if (-not $NoReport) {
    $reports = [ordered]@{
        "summary"     = "-report summary";
        "functions"   = "-report hotspots -group-by function -format csv -csv-delimiter tab";
        "modules"     = "-report hotspots -group-by module -format csv -csv-delimiter tab";
        "func-mod"    = "-report hotspots -group-by function,module -format csv -csv-delimiter tab";
        "threads"     = "-report hotspots -group-by thread -format csv -csv-delimiter tab";
        "thread-func" = "-report hotspots -group-by thread,function -format csv -csv-delimiter tab";
        # The client emits ITT task spans (VT_TRACE_EVENT: terrain/view_cull,
        # terrain/shadow_cull, terrain/mesh_upload) and counters (VT_TRACE_COUNTER:
        # ChunkSectionsVisible, mesh_capture_ns, ...). Without these two reports the
        # collector records every one of them and the export throws them all away,
        # leaving only whole-run function totals -- which cannot tell you what a
        # single frame cost, or how a cost moves when you turn the camera.
        "tasks"       = "-report hotspots -group-by task-type -format csv -csv-delimiter tab";
        "task-func"   = "-report hotspots -group-by task-type,function -format csv -csv-delimiter tab"
    }
    foreach ($k in $reports.Keys) {
        $ext = if ($k -eq "summary") { "txt" } else { "tsv" }
        $out = "$Root\$Name-$k.$ext"
        Write-Host "  report -> $out"
        Start-Process -FilePath $Vtune -ArgumentList "$($reports[$k]) -result-dir $shortResult" `
            -Wait -NoNewWindow -RedirectStandardOutput $out -RedirectStandardError "$Root\$Name-$k.err.log"
    }
}
Write-Host "DONE -- result dir: $resultDir"
