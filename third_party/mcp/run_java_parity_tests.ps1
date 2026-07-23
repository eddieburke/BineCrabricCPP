$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$javac = "C:\Program Files\Java\jdk1.8.0_111\bin\javac.exe"
if (-not (Test-Path $javac)) {
    $javac = "C:\Program Files\Java\jdk-23\bin\javac.exe"
}

$binDir = Join-Path $scriptDir "bin"
if (-not (Test-Path $binDir)) {
    New-Item -ItemType Directory -Path $binDir | Out-Null
}

Write-Host "Compiling Parity Test Suite against Java Minecraft Server classpath..." -ForegroundColor Cyan
& $javac -cp "jars/minecraft-named-original.jar;lib/*" -d $binDir src/net/minecraft/test/TestClientSocket.java src/net/minecraft/test/JavaParityTestRunner.java

Write-Host "Compilation successful." -ForegroundColor Green

$propsFile = Join-Path $scriptDir "server.properties"
Set-Content -Path $propsFile -Value "online-mode=false`nserver-port=25565`nspawn-protection=0`nview-distance=10" -Encoding ascii

$serverLogFile = Join-Path $scriptDir "server_out.log"
$serverErrFile = Join-Path $scriptDir "server_err.log"
if (Test-Path $serverLogFile) { Remove-Item $serverLogFile -Force }
if (Test-Path $serverErrFile) { Remove-Item $serverErrFile -Force }

Write-Host "Starting Official Java Minecraft Dedicated Server on port 25565 (online-mode=false)..." -ForegroundColor Cyan
$serverProcess = Start-Process -FilePath "java" -WorkingDirectory $scriptDir -ArgumentList "-cp `"jars/minecraft-server-original.jar;lib/*`" net.minecraft.server.MinecraftServer nogui" -RedirectStandardOutput $serverLogFile -RedirectStandardError $serverErrFile -PassThru

$serverReady = $false
for ($i = 0; $i -lt 90; $i++) {
    Start-Sleep -Seconds 1
    if (Test-Path $serverLogFile) {
        $content = Get-Content $serverLogFile -ErrorAction SilentlyContinue | Out-String
        if ($content -match "Done \(") {
            $serverReady = $true
            break
        }
    }
    if (Test-Path $serverErrFile) {
        $errContent = Get-Content $serverErrFile -ErrorAction SilentlyContinue | Out-String
        if ($errContent -match "Done \(") {
            $serverReady = $true
            break
        }
    }
}

if (-not $serverReady) {
    Write-Host "Warning: Timed out waiting for Java Server 'Done (' message, proceeding anyway..." -ForegroundColor Yellow
} else {
    Write-Host "Official Java Minecraft Server is ready and accepting network connections!" -ForegroundColor Green
}

try {
    Write-Host "Running Parity Tests against Official Java Minecraft Server..." -ForegroundColor Cyan
    & java -cp "jars/minecraft-named-original.jar;bin;lib/*" net.minecraft.test.JavaParityTestRunner 127.0.0.1 25565
}
finally {
    if ($serverProcess -and -not $serverProcess.HasExited) {
        Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
