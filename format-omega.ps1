# Token-dense clang-format pass (PowerShell 5 compatible).
# Uses native/.clang-format: ColumnLimit 0, Attach braces, 2-space indent.
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$clangFormatCandidates = @(
    "C:\Program Files\LLVM\bin\clang-format.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-format.exe"
)

$clangFormat = $null
foreach ($candidate in $clangFormatCandidates) {
    if (Test-Path $candidate) {
        $clangFormat = $candidate
        break
    }
}

if (-not $clangFormat) {
    throw "clang-format not found. Install LLVM or Visual Studio LLVM tools."
}

$extensions = @("*.cpp", "*.hpp", "*.h")
$roots = @("src", "tests")
$files = @()

foreach ($root in $roots) {
    if (-not (Test-Path $root)) {
        continue
    }
    foreach ($extension in $extensions) {
        $files += Get-ChildItem -Path $root -Recurse -Filter $extension -File
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
    throw ("clang-format failed on {0} file(s)." -f $failures)
}

Write-Host "clang-format complete."
