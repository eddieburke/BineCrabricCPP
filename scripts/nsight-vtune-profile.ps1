param(
 [string]$World = "",
 [int]$Seed = 31337,
 [int]$LoadSeconds = 5,
 [string]$Name = "profile",
 [switch]$Direct,
 [switch]$DebugHud,
 [switch]$NoReport
)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Exe = "$Root\build-omega\minecraft_native.exe"
$VtuneDir = "$Root\build-omega\vtune-$Name"
$NsysOut = "$Root\build-omega\nsys-$Name"
$NsysRep = "$NsysOut.nsys-rep"
$NsysSqlite = "$NsysOut.sqlite"
if (-not (Test-Path -LiteralPath $Exe)) { Write-Host "Missing $Exe" -ForegroundColor Red; exit 1 }
$Vtune = Get-ChildItem "C:\Program Files (x86)\Intel\oneAPI\vtune\*\bin64\vtune.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
if ($Vtune -eq $null) { Write-Host "Missing VTune (checked C:\Program Files (x86)\Intel\oneAPI\vtune)" -ForegroundColor Red; exit 1 }
$Vtune = $Vtune.FullName
$Nsys = Get-ChildItem "C:\Program Files\NVIDIA Corporation\Nsight Systems*\target-windows-x64\nsys.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
if ($Nsys -eq $null) { Write-Host "Missing Nsight Systems (checked C:\Program Files\NVIDIA Corporation\Nsight Systems)" -ForegroundColor Red; exit 1 }
$Nsys = $Nsys.FullName
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { Write-Host "Not Administrator: CPU sampling will be disabled. Right-click PowerShell -> Run as administrator." -ForegroundColor Yellow }
$optionsPath = "$env:APPDATA\.minecraft\options.txt"
if (Test-Path -LiteralPath $optionsPath) {
 $opts = @{}
 foreach ($line in Get-Content -LiteralPath $optionsPath) { $kv = $line -split ":", 2; if ($kv.Count -eq 2) { $opts[$kv[0]] = $kv[1] } }
 if ($opts.ContainsKey("viewDistance") -and $opts.ContainsKey("renderScale")) {
  $vd = [int]$opts["viewDistance"]
  $rs = [double]$opts["renderScale"]
  $blocks = (256 -shr $vd) * $rs
  Write-Host ("Render distance: renderScale={0}, viewDistance={1} -> {2} blocks ({3} chunks)" -f $rs, $vd, $blocks, [math]::Floor(($blocks + 8) / 16))
 }
}
if (Test-Path -LiteralPath $VtuneDir) { Remove-Item -Recurse -Force -LiteralPath $VtuneDir }
if (Test-Path -LiteralPath $NsysRep) { Remove-Item -Force -LiteralPath $NsysRep }
if (Test-Path -LiteralPath $NsysSqlite) { Remove-Item -Force -LiteralPath $NsysSqlite }
foreach ($old in @("$NsysOut-nvtx_sum.csv", "$NsysOut-osrt_sum.csv", "$NsysOut-cuda_api_sum.csv")) { if (Test-Path -LiteralPath $old) { Remove-Item -Force -LiteralPath $old } }
$gameArgs = @()
if ($Direct) {
 if ($World -eq "") { $World = "World" }
 $gameArgs = @("--world", $World, "--seed", "$Seed")
}
if ($DebugHud) { $gameArgs += "--debug-hud" }
Write-Host "=== VTune CPU hotspots (target-pid) ==="
if ($Direct) { Write-Host "Game: $Exe --world $World --seed $Seed (direct)" } else { Write-Host "Game: $Exe (menu start -- pick World from menu)" }
$game = Start-Process -FilePath $Exe -ArgumentList $gameArgs -PassThru -WorkingDirectory "$Root\build-omega"
Write-Host "Launched PID $($game.Id) -- loading for ${LoadSeconds}s"
Start-Sleep -Seconds $LoadSeconds
$game.Refresh()
if ($game.HasExited) { Write-Host "Game exited early during load, exit code $($game.ExitCode)" -ForegroundColor Red; exit 1 }
$collectArgs = @("-collect", "hotspots", "-knob", "sampling-mode=sw", "-knob", "enable-stack-collection=true", "-data-limit=20000", "-result-dir", $VtuneDir, "-target-pid", "$($game.Id)")
$vtProc = Start-Process -FilePath $Vtune -ArgumentList $collectArgs -PassThru -RedirectStandardOutput "$Root\vtune-$Name.log" -RedirectStandardError "$Root\vtune-$Name.err.log"
Write-Host "VTune attached (PID $($vtProc.Id)). Play normally, close the game when done -- collecting until you exit."
$game.WaitForExit()
Write-Host "Game closed. Stopping VTune..."
$vtProc.Refresh()
if (-not $vtProc.HasExited) {
 if (-not $vtProc.WaitForExit(20000)) {
  Start-Process -FilePath $Vtune -ArgumentList @("-command", "stop", "-result-dir", $VtuneDir) -Wait -NoNewWindow -RedirectStandardOutput "$Root\vtune-$Name-stop.log" -RedirectStandardError "$Root\vtune-$Name-stop.err.log"
  $vtProc.WaitForExit()
 }
}
Write-Host "VTune exited with code $($vtProc.ExitCode)"
if (-not $NoReport) {
 $reports = [ordered]@{
  "summary" = "-report summary"
  "functions" = "-report hotspots -group-by function -format csv -csv-delimiter tab"
  "modules" = "-report hotspots -group-by module -format csv -csv-delimiter tab"
  "func-mod" = "-report hotspots -group-by function,module -format csv -csv-delimiter tab"
  "threads" = "-report hotspots -group-by thread -format csv -csv-delimiter tab"
  "thread-func" = "-report hotspots -group-by thread,function -format csv -csv-delimiter tab"
 }
 foreach ($k in $reports.Keys) {
  $ext = if ($k -eq "summary") { "txt" } else { "tsv" }
  $out = "$Root\vtune-$Name-$k.$ext"
  if (Test-Path -LiteralPath $out) { Remove-Item -Force -LiteralPath $out }
  Write-Host "  report -> $out"
  Start-Process -FilePath $Vtune -ArgumentList (($reports[$k] -split " ") + @("-result-dir", $VtuneDir)) -Wait -NoNewWindow -RedirectStandardOutput $out -RedirectStandardError "$Root\vtune-$Name-$k.err.log"
 }
}
Write-Host ""
Write-Host "=== Nsight Systems (GPU timeline + CPU samples) ==="
if ($Direct) { Write-Host "Relaunching game under nsys: $NsysRep (replaces previous, direct --world $World)" } else { Write-Host "Relaunching game under nsys: $NsysRep (replaces previous, menu start -- pick World from menu)" }
Write-Host "Play normally, close the game when done."
& $Nsys profile -s process-tree --cpuctxsw=process-tree --trace=opengl -o $NsysOut --force-overwrite=true $Exe $gameArgs
if ($LASTEXITCODE -ne 0) { Write-Host "nsys profile exit $LASTEXITCODE" -ForegroundColor Yellow }
if (Test-Path -LiteralPath $NsysRep) {
 Write-Host "Rendering stats (force-export, replaces stale sqlite)..."
 & $Nsys stats --force-export=true $NsysRep
}
Write-Host "DONE -- VTune dir: $VtuneDir -- Nsight: $NsysRep"
