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
function DropLines($desc, $state, [int[]]$lines) {
  $kept = New-Object System.Collections.Generic.List[string]
  $all = $state.text -split "`n"
  $lineSet = New-Object 'System.Collections.Generic.HashSet[int]'
  foreach($l in $lines) { [void]$lineSet.Add($l) }
  for($i = 0; $i -lt $all.Count; $i++) {
    if(-not $lineSet.Contains($i + 1)) { $kept.Add($all[$i]) }
  }
  $before = $state.text
  $state.text = [string]::Join("`n", $kept)
  $removed = ([regex]::Matches($before, "`n")).Count - ([regex]::Matches($state.text, "`n")).Count
  if($removed -eq $lines.Count) { $script:report.Add("OK   $desc (dropped $removed lines)") }
  else { $script:report.Add("FAIL $desc (dropped $removed of $($lines.Count))") }
}
function DropRange($desc, $state, [int]$from, [int]$to) {
  $all = $state.text -split "`n"
  if($all.Count -lt $to) { $script:report.Add("FAIL $desc (file too short)"); return }
  $check = ($all[($from-1)..($to-1)] | ForEach-Object { $_.Trim() }) -join ' | '
  if($check -notmatch '^//|^/\*') { $script:report.Add("FAIL $desc (lines not comments: $check)"); return }
  DropLines $desc $state @($from..$to)
}

# Tessellator.cpp
$s = LoadNormalized 'client\render\Tessellator.cpp'
DropRange 'Tess double-cast' $s 294 294
DropRange 'Tess iris parity' $s 298 301
DropRange 'Tess mc_entity prose 1' $s 345 345
DropRange 'Tess mc_entity prose 2' $s 351 351
DropRange 'Tess last triangle' $s 381 381
DropRange 'Tess chunkOffset 408' $s 408 408
DropRange 'Tess quad index' $s 433 434
DropRange 'Tess chunkOffset 454' $s 454 454
DropRange 'Tess VBO copy' $s 456 457
DropRange 'Tess chunkOffset 490' $s 490 490
DropRange 'Tess chunkOffset 517' $s 517 517
Save $s

# Manager.cpp
$s = LoadNormalized 'client\render\pipeline\Manager.cpp'
DropRange 'Manager profile baseline' $s 398 402
DropRange 'Manager profile preset' $s 419 421
DropRange 'Manager re-run parse' $s 579 586
Save $s

# Pipeline.cpp
$s = LoadNormalized 'client\render\pipeline\Pipeline.cpp'
DropRange 'Pipeline ColorWheel fmt' $s 63 65
DropRange 'Pipeline clrwl on-demand' $s 639 640
Save $s

# CoreGlslTransformer.cpp
$s = LoadNormalized 'client\render\shaders\CoreGlslTransformer.cpp'
DropRange 'Transformer gl_FragData' $s 175 179
DropRange 'Transformer bias overload' $s 280 281
DropRange 'Transformer clamp bounds' $s 341 346
Save $s

# GlslSource.hpp
$s = LoadNormalized 'client\render\shaders\GlslSource.hpp'
DropRange 'GlslSource mask comment' $s 9 12
Save $s

# WorldChunks.cpp
$s = LoadNormalized 'world\WorldChunks.cpp'
DropRange 'WorldChunks sky-color' $s 277 277
Save $s

# ChunkSource.hpp
$s = LoadNormalized 'world\chunk\ChunkSource.hpp'
DropRange 'ChunkSource progress comment' $s 10 11
Save $s

# ChunkSectionSystem.hpp
$s = LoadNormalized 'client\render\world\ChunkSectionSystem.hpp'
DropRange 'ChunkSectionSystem frontier comment' $s 97 97
Save $s

$report | ForEach-Object { $_ }
Write-Output "TOTAL: $($report.Count), $(( $report | Where-Object { $_ -like 'FAIL*' } ).Count) failures"
