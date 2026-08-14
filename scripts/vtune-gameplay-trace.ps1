$Root = "C:\Users\Eddie\Documents\New project 2"
$Exe = "$Root\build-vtune\minecraft_native.exe"
$Vtune = "C:\Program Files (x86)\Intel\oneAPI\vtune\2026.4\bin64\vtune.exe"
$fso = New-Object -ComObject Scripting.FileSystemObject
$shortRoot = $fso.GetFolder($Root).ShortPath
$shortResult = "$shortRoot\vtune-result6"

Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@

$proc = Start-Process -FilePath $Exe -ArgumentList @("--world","streamstress","--seed","31337","--debug-hud") -PassThru
Write-Host "Launched PID $($proc.Id)"

Start-Sleep -Seconds 8
$proc.Refresh()
if ($proc.HasExited) {
    Write-Host "Game exited early during load, exit code $($proc.ExitCode)"
    exit 1
}
[Win32]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Start-Sleep -Seconds 1

$vtuneProc = Start-Process -FilePath $Vtune -ArgumentList "-collect hotspots -knob enable-stack-collection=true -duration 75 -result-dir $shortResult -target-pid $($proc.Id)" -RedirectStandardOutput "$Root\vtune-collect6.log" -RedirectStandardError "$Root\vtune-collect6.err.log" -PassThru
Write-Host "VTune attached, PID $($vtuneProc.Id)"

$deadline = (Get-Date).AddSeconds(72)
while ((Get-Date) -lt $deadline) {
    [Win32]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
    [System.Windows.Forms.SendKeys]::SendWait("w")
    Start-Sleep -Milliseconds 120
    [System.Windows.Forms.SendKeys]::SendWait(" ")
    Start-Sleep -Milliseconds 120
}
Write-Host "Movement simulation done, waiting for vtune to finalize..."
$vtuneProc.WaitForExit()
Write-Host "vtune exited with code $($vtuneProc.ExitCode)"

Start-Sleep -Seconds 2
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Write-Host "DONE"
