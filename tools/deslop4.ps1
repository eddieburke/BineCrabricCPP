$ErrorActionPreference = 'Stop'
$root = 'C:\Users\Eddie\Documents\New project 2\src\net\minecraft'
$report = [System.Collections.Generic.List[string]]::new()
function LoadNormalized($rel) {
  $p = Join-Path $root $rel
  $raw = [System.IO.File]::ReadAllText($p)
  $crlf = ([regex]::Matches($raw, "`r`n")).Count
  $lfOnly = ([regex]::Matches($raw, "(?<!`r)`n")).Count
  $eol = if($crlf -gt $lfOnly) { "`r`n" } else { "`n" }
  $t = $raw -replace "`r`n", "`n"
  return @{ text = $t; eol = $eol; path = $p }
}
function Save($state) {
  $out = $state.text -replace "`n", $state.eol
  [System.IO.File]::WriteAllText($state.path, $out, [System.Text.UTF8Encoding]::new($false))
}
function DropByContent($desc, $state, [string]$needle, [int]$expected = 1) {
  $lines = $state.text -split "`n"
  $removed = 0
  $kept = New-Object System.Collections.Generic.List[string]
  foreach($line in $lines) {
    if($line.TrimStart().StartsWith($needle)) { $removed++ }
    else { $kept.Add($line) }
  }
  if($removed -ne $expected) { $script:report.Add("FAIL $desc (expected=$expected removed=$removed)"); return }
  $state.text = [string]::Join("`n", $kept)
  $script:report.Add("OK   $desc (removed $removed)")
}
function DropLinesStartingWith($desc, $state, [string[]]$needles) {
  $lines = $state.text -split "`n"
  $kept = New-Object System.Collections.Generic.List[string]
  $removed = 0
  foreach($line in $lines) {
    $drop = $false
    foreach($n in $needles) { if($line.TrimStart().StartsWith($n)) { $drop = $true; break } }
    if($drop) { $removed++ } else { $kept.Add($line) }
  }
  if($removed -eq 0) { $script:report.Add("FAIL $desc (nothing removed)"); return }
  $state.text = [string]::Join("`n", $kept)
  $script:report.Add("OK   $desc (removed $removed lines)")
}

$s = LoadNormalized 'client\render\Tessellator.cpp'
DropLinesStartingWith 'Tess misc comments' $s @('// Add in double, then cast once', '// Iris parity:', '// setDrawPose). Positions take', '// here so the packed byte normal', '// extra work - finishQuad', '// GL_TRIANGLES / GL_TRIANGLE_STRIP - fill', '// Apply pending section-local chunkOffset', '// Meshes captured in quad mode', '// shared quad index buffer', '// Meshes carry their pose', '// VBO copy is always safe', '// x = block id; y = 1.0 fluids', '// Non-terrain geometry must not look like fluid')
Save $s

$s = LoadNormalized 'client\render\pipeline\Manager.cpp'
DropLinesStartingWith 'Manager profile preset' $s @('// Selecting a profile applies the whole preset', '// pack''s shipped defaults, then lay', '// Individual options changed after the fact')
Save $s

$s = LoadNormalized 'client\render\pipeline\Pipeline.cpp'
DropLinesStartingWith 'Pipeline clrwl on-demand' $s @('// ColorWheel material programs are compiled on demand')
Save $s

$s = LoadNormalized 'client\render\shaders\CoreGlslTransformer.cpp'
DropLinesStartingWith 'Transformer bias overload' $s @('// The bias overload of texture()')
Save $s

$report | ForEach-Object { $_ }
Write-Output "TOTAL: $($report.Count), $(( $report | Where-Object { $_ -like 'FAIL*' } ).Count) failures"
