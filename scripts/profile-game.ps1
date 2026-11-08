param(
 [string]$World = "",
 [int]$Seed = 31337,
 [int]$LoadSeconds = 5,
 [string]$Name = "profile",
 [switch]$Direct,
 [switch]$DebugHud,
 [switch]$NoReport,
 [switch]$SkipVtune,
 [switch]$SkipNsight
)
$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
if ([string]::IsNullOrEmpty($ScriptDir)) { $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path }
$Root = Split-Path -Parent $ScriptDir
if (-not (Test-Path -LiteralPath "$Root\build-omega.ps1")) {
 $cand = Get-Item -LiteralPath $ScriptDir | Select-Object -ExpandProperty FullName
 while ($cand -and $cand.Length -gt 3) {
  if (Test-Path -LiteralPath "$cand\build-omega.ps1") { $Root = $cand; break }
  $cand = Split-Path -Parent $cand
 }
}
$Exe = Join-Path $Root "build-omega\minecraft_native.exe"
$VtuneDir = Join-Path $Root "build-omega\vtune-$Name"
$NsysOut = Join-Path $Root "build-omega\nsys-$Name"
$NsysRep = "$NsysOut.nsys-rep"
$NsysSqlite = "$NsysOut.sqlite"
if (-not (Test-Path -LiteralPath $Exe)) { Write-Host "Missing $Exe" -ForegroundColor Red; exit 1 }
function Find-Vtune {
 $c = Get-ChildItem "C:\Program Files (x86)\Intel\oneAPI\vtune\*\bin64\vtune.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
 if ($c) { return $c.FullName }
 return $null
}
function Find-Nsys {
 $c = Get-ChildItem "C:\Program Files\NVIDIA Corporation\Nsight Systems*\target-windows-x64\nsys.exe" -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
 if ($c) { return $c.FullName }
 return $null
}
$Vtune = Find-Vtune
$Nsys = Find-Nsys
if (-not $SkipVtune -and $Vtune -eq $null) { Write-Host "Missing VTune (checked C:\Program Files (x86)\Intel\oneAPI\vtune) -- skipping VTune phase" -ForegroundColor Yellow; $SkipVtune = $true }
if (-not $SkipNsight -and $Nsys -eq $null) { Write-Host "Missing Nsight Systems (checked C:\Program Files\NVIDIA Corporation\Nsight Systems*) -- skipping Nsight phase" -ForegroundColor Yellow; $SkipNsight = $true }
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { Write-Host "Not Administrator: CPU sampling will be disabled. Right-click PowerShell -> Run as administrator." -ForegroundColor Yellow }
$optionsPath = Join-Path $env:APPDATA ".minecraft\options.txt"
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
if (Test-Path -LiteralPath $VtuneDir) { Remove-Item -Recurse -Force -LiteralPath $VtuneDir -ErrorAction SilentlyContinue }
if (Test-Path -LiteralPath $NsysRep) { Remove-Item -Force -LiteralPath $NsysRep -ErrorAction SilentlyContinue }
if (Test-Path -LiteralPath $NsysSqlite) { Remove-Item -Force -LiteralPath $NsysSqlite -ErrorAction SilentlyContinue }
foreach ($old in @("$Root\vtune-$Name.log", "$Root\vtune-$Name.err.log", "$Root\vtune-$Name-summary.txt", "$Root\vtune-$Name-functions.tsv", "$Root\vtune-$Name-modules.tsv", "$Root\vtune-$Name-threads.tsv")) { if (Test-Path -LiteralPath $old) { Remove-Item -Force -LiteralPath $old -ErrorAction SilentlyContinue } }
$gameArgs = @()
if ($Direct) {
 if ($World -eq "") { $World = "World" }
 $gameArgs = @("--world", $World, "--seed", "$Seed")
}
if ($DebugHud) { $gameArgs += "--debug-hud" }
if (-not $SkipVtune) {
 Write-Host "=== VTune CPU hotspots (target-pid) ==="
 if ($Direct) { Write-Host "Game: $Exe --world $World --seed $Seed (direct)" } else { Write-Host "Game: $Exe (menu start -- pick World from menu)" }
 $game = Start-Process -FilePath $Exe -ArgumentList $gameArgs -PassThru -WorkingDirectory (Join-Path $Root "build-omega")
 Write-Host "Launched PID $($game.Id) -- loading for ${LoadSeconds}s"
 Start-Sleep -Seconds $LoadSeconds
 $game.Refresh()
 if ($game.HasExited) { Write-Host "Game exited early during load, exit code $($game.ExitCode)" -ForegroundColor Red; exit 1 }
 $collectArgs = @("-collect", "hotspots", "-knob", "sampling-mode=sw", "-knob", "enable-stack-collection=true", "-data-limit=20000", "-result-dir", $VtuneDir, "-target-pid", "$($game.Id)")
 $vtProc = Start-Process -FilePath $Vtune -ArgumentList $collectArgs -PassThru -RedirectStandardOutput (Join-Path $Root "vtune-$Name.log") -RedirectStandardError (Join-Path $Root "vtune-$Name.err.log")
 Write-Host "VTune $Vtune attached (PID $($vtProc.Id)). Play normally, close the game when done -- collecting until you exit."
 $game.WaitForExit()
 Write-Host "Game closed. Stopping VTune..."
 $vtProc.Refresh()
 if (-not $vtProc.HasExited) {
  if (-not $vtProc.WaitForExit(20000)) {
   Start-Process -FilePath $Vtune -ArgumentList @("-command", "stop", "-result-dir", $VtuneDir) -Wait -NoNewWindow -RedirectStandardOutput (Join-Path $Root "vtune-$Name-stop.log") -RedirectStandardError (Join-Path $Root "vtune-$Name-stop.err.log")
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
   $out = Join-Path $Root "vtune-$Name-$k.$ext"
   if (Test-Path -LiteralPath $out) { Remove-Item -Force -LiteralPath $out }
   Write-Host "  report -> $out"
   Start-Process -FilePath $Vtune -ArgumentList (($reports[$k] -split " ") + @("-result-dir", $VtuneDir)) -Wait -NoNewWindow -RedirectStandardOutput $out -RedirectStandardError (Join-Path $Root "vtune-$Name-$k.err.log")
  }
 }
 Write-Host ""
}
if (-not $SkipNsight) {
 Write-Host "=== Nsight Systems (GPU timeline + CPU samples) ==="
 if ($Direct) { Write-Host "Relaunching under nsys: $NsysRep (fresh, replaces previous, direct --world $World)" } else { Write-Host "Relaunching under nsys: $NsysRep (fresh, replaces previous, menu start -- pick World from menu)" }
 Write-Host "Nsys: $Nsys"
 Write-Host "Play normally, close the game when done."
 $before = Get-Date
 & $Nsys profile -s process-tree --cpuctxsw=process-tree --trace=opengl -o $NsysOut --force-overwrite=true $Exe @gameArgs
 if ($LASTEXITCODE -ne 0) { Write-Host "nsys profile exit $LASTEXITCODE" -ForegroundColor Yellow }
 if (-not (Test-Path -LiteralPath $NsysRep)) { Write-Host "No .nsys-rep produced at $NsysRep" -ForegroundColor Red }
 else {
  $ts = (Get-Item -LiteralPath $NsysRep).LastWriteTime
  if ($ts -lt $before.AddSeconds(-5)) { Write-Host "Warning: $NsysRep timestamp is stale ($ts), profile may have reused old data" -ForegroundColor Yellow }
  Write-Host "Rendering stats (force-export, replaces stale sqlite)..."
  & $Nsys stats --force-export=true $NsysRep
 }
}
Write-Host "DONE -- VTune dir: $VtuneDir -- Nsight: $NsysRep"
