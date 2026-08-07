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
function NukeComments($desc, $state, [string[]]$keepPrefixes) {
  $lines = $state.text -split "`n"
  $kept = New-Object System.Collections.Generic.List[string]
  $removed = 0
  foreach($line in $lines) {
    $trimmed = $line.TrimStart()
    if($trimmed.StartsWith('//')) {
      $keep = $false
      foreach($k in $keepPrefixes) { if($trimmed.StartsWith($k)) { $keep = $true; break } }
      if($keep) { $kept.Add($line) } else { $removed++ }
    } else { $kept.Add($line) }
  }
  $state.text = [string]::Join("`n", $kept)
  $script:report.Add("OK   $desc (removed $removed comment lines)")
}

$s = LoadNormalized 'client\render\Tessellator.cpp'
NukeComments 'Tessellator.cpp' $s @('// https://shaders.properties/current/reference/attributes/mc_entity/')
Save $s

$s = LoadNormalized 'client\render\pipeline\Manager.cpp'
NukeComments 'Manager.cpp' $s @('// Applies blend/alphaTest directives by the resolved source name', '// (ProgramDirectives.java:80-83).', '// ColorWheel: the holder carries LabPBR-mipmapped companion textures', '// (IrisRenderingPipeline.java:848).', '// https://github.com/IrisShaders/Iris/blob/', '// Java CommonUniforms.atlasSize (CommonUniforms.java:81-93) reports (0,0)', '// for any texture it has not uploaded itself.')
Save $s

$s = LoadNormalized 'client\render\pipeline\Pipeline.cpp'
NukeComments 'Pipeline.cpp' $s @()
Save $s

$s = LoadNormalized 'client\render\shaders\CoreGlslTransformer.cpp'
NukeComments 'CoreGlslTransformer.cpp' $s @('// https://github.com/IrisShaders/Iris/blob/', '// SodiumTransformer.java:124')
Save $s

$s = LoadNormalized 'client\render\chunk\ChunkBuilder.hpp'
NukeComments 'ChunkBuilder.hpp' $s @()
Save $s

$report | ForEach-Object { $_ }
