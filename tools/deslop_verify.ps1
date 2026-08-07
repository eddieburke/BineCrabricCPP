$ErrorActionPreference = 'Stop'
$root = 'C:\Users\Eddie\Documents\New project 2\src\net\minecraft'
$results = [System.Collections.Generic.List[string]]::new()
function Get-C($p) { return [System.IO.File]::ReadAllText($p) }
function Set-C($p, $t) { [System.IO.File]::WriteAllText($p, $t, [System.Text.UTF8Encoding]::new($false)) }
function Check($desc, $rel, $needle, [bool]$shouldExist = $true) {
  $p = Join-Path $root $rel
  $t = Get-C $p
  $found = $t.Contains($needle)
  if($found -eq $shouldExist) { $script:results.Add("OK   $desc") }
  else { $script:results.Add("FAIL $desc (needle=$(if($shouldExist){'present'}else{'absent'}) found=$found)") }
}
function CountCheck($desc, $rel, $pattern, [int]$expected) {
  $p = Join-Path $root $rel
  $t = Get-C $p
  $n = ([regex]::Matches($t, $pattern)).Count
  if($n -eq $expected) { $script:results.Add("OK   $desc (x$n)") }
  else { $script:results.Add("FAIL $desc (expected=$expected found=$n)") }
}
function RemoveAll($desc, $rel, $pattern, [int]$expected) {
  $p = Join-Path $root $rel
  $t = Get-C $p
  $rx = [regex]::new($pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)
  $n = $rx.Matches($t).Count
  if($n -ne $expected) { $script:results.Add("FAIL $desc (expected=$expected found=$n)"); return }
  $t = $rx.Replace($t, '')
  Set-C $p $t
  $script:results.Add("OK   $desc (removed x$n)")
}

# --- structural verifications (from this session's edits) ---
Check 'ChunkBuilder neighbors comment gone' 'client\render\chunk\ChunkBuilder.hpp' 'maintained by ChunkSectionSystem' $false
Check 'ChunkSectionSystem sectionsChanged_ present' 'client\render\world\ChunkSectionSystem.hpp' 'bool sectionsChanged_ = true;'
CountCheck 'ChunkSectionSystem hpp no stray comments' 'client\render\world\ChunkSectionSystem.hpp' '(?m)^\s*//' 0
CountCheck 'ChunkSource hpp comment-free getChunkIfLoaded' 'world\chunk\ChunkSource.hpp' '(?m)^\s*//' 0
CountCheck 'ChunkCache hpp one getChunkIfLoaded' 'world\chunk\ChunkCache.hpp' 'getChunkIfLoaded' 1
CountCheck 'ChunkCache cpp one getChunkIfLoaded impl' 'world\chunk\ChunkCache.cpp' 'ChunkCache::getChunkIfLoaded' 1
Check 'EntityRenderer no g_lastTexturePath' 'client\render\entity\EntityRenderer.cpp' 'g_lastTexturePath' $false
Check 'EntityRenderer has local statics' 'client\render\entity\EntityRenderer.cpp' 'static std::string lastTexturePath;'
Check 'EntityRenderer diffuse comment gone' 'client\render\entity\EntityRenderer.cpp' 'Diffuse is unit 0' $false
Check 'Dispatcher no g_lastEntityName' 'client\render\entity\EntityRenderDispatcher.cpp' 'g_lastEntityName' $false
Check 'Dispatcher has local statics' 'client\render\entity\EntityRenderDispatcher.cpp' 'static std::string lastEntityName;'
Check 'Dispatcher comment gone' 'client\render\entity\EntityRenderDispatcher.cpp' 'Java resolves the current entity' $false
Check 'Tessellator.hpp has ScopedBatch' 'client\render\Tessellator.hpp' 'class ScopedBatch'
CountCheck 'Tessellator.hpp comments gone' 'client\render\Tessellator.hpp' '(?m)^\s*//' 0
Check 'Tessellator.cpp beginPart impl' 'client\render\Tessellator.cpp' 'void Tessellator::beginPart(int mode)'
Check 'Tessellator.cpp reset comment gone' 'client\render\Tessellator.cpp' 'Only the local snapshot is dropped' $false
Check 'Tessellator.cpp start snapshot comment gone' 'client\render\Tessellator.cpp' 'Snapshot the pose for this batch' $false
Check 'Instance.hpp no prewarmQueued' 'client\render\pipeline\Instance.hpp' 'prewarmQueued' $false
Check 'Instance.hpp queue fields' 'client\render\pipeline\Instance.hpp' 'std::vector<std::string> prewarmQueue;'
CountCheck 'Compiler.hpp no comments' 'client\render\shaders\Compiler.hpp' '(?m)^\s*//' 0
Check 'Compiler.hpp single-arg prewarmStep' 'client\render\shaders\Compiler.hpp' 'static bool prewarmStep(PackInstance& pack, const LogFnLevel& logOnce);'
Check 'Compiler.cpp buildPrewarmQueue impl' 'client\render\shaders\Compiler.cpp' 'void PackCompiler::buildPrewarmQueue(PackInstance& pack) {'
CountCheck 'Compiler.cpp prewarmStep single arg' 'client\render\shaders\Compiler.cpp' 'prewarmStep\(PackInstance& pack, const LogFnLevel& logOnce\)' 2
Check 'Compiler.cpp prewarm-still-queued comment gone' 'client\render\shaders\Compiler.cpp' 'Prewarm still has programs queued' $false
Check 'Compiler.cpp prewarm-already-prepared comment gone' 'client\render\shaders\Compiler.cpp' 'Prewarm already prepared' $false
Check 'Manager.hpp logOnce private' 'client\render\pipeline\Manager.hpp' 'private:' 
Check 'Manager.hpp logFn member' 'client\render\pipeline\Manager.hpp' 'LogFnLevel logFn();'
Check 'Manager.cpp no packLogFn' 'client\render\pipeline\Manager.cpp' 'packLogFn' $false
CountCheck 'Manager.cpp no prewarm constants' 'client\render\pipeline\Manager.cpp' 'kPrewarmMaxProgramsPerFrame' 0
Check 'Manager.cpp logFn impl' 'client\render\pipeline\Manager.cpp' 'PackCompiler::LogFnLevel PackManager::logFn() {'
Check 'Manager.cpp prewarmQueue.clear in setSettings' 'client\render\pipeline\Manager.cpp' 'target->prewarmQueue.clear();'
Check 'Manager.cpp profile baseline comment gone' 'client\render\pipeline\Manager.cpp' 'selected profile''s values become the baseline' $false
Check 'Manager.cpp profile preset comment gone' 'client\render\pipeline\Manager.cpp' 'Selecting a profile applies the whole preset' $false
Check 'Manager.cpp staged re-parse comment gone' 'client\render\pipeline\Manager.cpp' 'Stage a fully re-parsed pack' $false
Check 'Manager.cpp re-run parse comment gone' 'client\render\pipeline\Manager.cpp' 'Re-run the parse with the CURRENT settings' $false
Check 'Pipeline.cpp budgeted comment gone' 'client\render\pipeline\Pipeline.cpp' 'Budgeted: a cold pack' $false
CountCheck 'Pipeline.cpp prewarmStep 1-arg' 'client\render\pipeline\Pipeline.cpp' 'prewarmStep\(\*pack,' 1
Check 'CoreGlslTransformer no sanitize' 'client\render\shaders\CoreGlslTransformer.cpp' 'sanitizeMismatchedClampOverloads' $false
Check 'CoreGlslTransformer one-mask comment gone' 'client\render\shaders\CoreGlslTransformer.cpp' 'One mask serves the whole declaration batch' $false
CountCheck 'CoreGlslTransformer CodeMask uses' 'client\render\shaders\CoreGlslTransformer.cpp' '\bCodeMask\b' 4
Check 'GlslSource.hpp CodeMask alias' 'client\render\shaders\GlslSource.hpp' 'using CodeMask = std::vector<unsigned char>;'
CountCheck 'GlslSource.hpp no comments' 'client\render\shaders\GlslSource.hpp' '(?m)^\s*//' 0
CountCheck 'GlslSource.cpp no vector<bool>' 'client\render\shaders\GlslSource.cpp' 'vector<bool>' 0
CountCheck 'CoreGlslTransformer no vector<bool>' 'client\render\shaders\CoreGlslTransformer.cpp' 'vector<bool>' 0

# --- remaining comment purges ---
RemoveAll 'Compiler.cpp rememberDrawBuffers comment' 'client\render\shaders\Compiler.cpp' '(?m)^\s*// Draw buffer indices live in the prepared fragment source.*?\n(?:.*?\n){0,3}\s*// apply them without preparing.*?$\n?' 1 2>$null
RemoveAll 'Compiler.cpp embedded vanilla comment' 'client\render\shaders\Compiler.cpp' '(?ms)^\s*// Embedded vanilla sources are baked fully resolved at configure time.*?strips comments\), so\n\s*// the include machinery never runs for the built-in pack\.\n' 1
RemoveAll 'Instance.hpp cache key comment' 'client\render\pipeline\Instance.hpp' '(?ms)^\s*// What prewarm already worked out for each program.*?program name -> ProgramCache key\.\n' 1
RemoveAll 'Instance.hpp draw buffers comment' 'client\render\pipeline\Instance.hpp' '(?ms)^\s*// ProgramCache key -> draw buffer indices parsed out of the prepared fragment\.\n' 1
RemoveAll 'Instance.hpp compiler owner comment' 'client\render\pipeline\Instance.hpp' '(?ms)^\s*// Owned by the owner of this instance.*?shares one cache directory\.\n' 1
RemoveAll 'Transformer gl_FragData comment' 'client\render\shaders\CoreGlslTransformer.cpp' '(?ms)^\s*// gl_FragData\[N\] writes DRAW BUFFER N.*?legacy path only\.\n' 1
RemoveAll 'Transformer safe-to-parse comment' 'client\render\shaders\CoreGlslTransformer.cpp' '(?m)^\s*// Safe to parse fatally: Pipeline builds the scene targets from these same strings\.\n' 1
RemoveAll 'Transformer bias overload comment' 'client\render\shaders\CoreGlslTransformer.cpp' '(?ms)^\s*// The bias overload of texture\(\) is fragment-stage-only.*?dropped there\.\n' 1
RemoveAll 'Transformer clamp bounds comment' 'client\render\shaders\CoreGlslTransformer.cpp' '(?ms)^\s*// Strict GLSL compilers reject clamp/min/max calls.*?are left untouched\.\n' 1
RemoveAll 'Transformer fog globals comment' 'client\render\shaders\CoreGlslTransformer.cpp' '(?ms)^\s*// gl_Fog\.start/\.end/\.scale/\.color were fixed-function builtins.*?live fog uniforms instead\.\n' 1
RemoveAll 'WorldChunks sky-color comment' 'world\WorldChunks.cpp' '(?m)^\s*// Java uses the raw sky-color temperature sampler here, not transformed biome climate\.\n' 1
RemoveAll 'Manager vanilla-pack comment' 'client\render\pipeline\Manager.cpp' '(?ms)^\s*// The vanilla pack ships inside the executable.*?tweaked in place\.\n' 1
RemoveAll 'Manager vanilla empty-id comment' 'client\render\pipeline\Manager.cpp' '(?ms)^\s*// The vanilla definition carries empty id maps.*?without a pack-presence branch\.\n' 1
RemoveAll 'Tessellator identity-pose comment' 'client\render\Tessellator.cpp' '(?ms)^\s*// An identity pose means the producer already emits pass-base-space positions.*?largest vertex counts\.\n' 1
RemoveAll 'Tessellator defaults comment' 'client\render\Tessellator.cpp' '(?ms)^\s*// Defaults in for vertices that never got their tangent/mid-tex filled.*?did the same\)\.\n' 1
RemoveAll 'Tessellator double-cast comment' 'client\render\Tessellator.cpp' '(?m)^\s*// Add in double, then cast once - avoids float\(world\) \+ float\(offset\) collapse at far coords\.\n' 1
RemoveAll 'Tessellator iris-parity comment' 'client\render\Tessellator.cpp' '(?ms)^\s*// Iris parity: the pose lives in the vertices.*?from the \(posed\) corners\.\n' 1
RemoveAll 'Tessellator vaUV2 comment' 'client\render\Tessellator.cpp' '(?ms)^\s*// vaUV2 is an unnormalized ushort2 of level\*16.*?snapping to a whole level\.\n' 1
RemoveAll 'Tessellator last-triangle comment' 'client\render\Tessellator.cpp' '(?m)^\s*// GL_TRIANGLES / GL_TRIANGLE_STRIP - fill last triangle\.\n' 1
RemoveAll 'Tessellator chunkOffset comments' 'client\render\Tessellator.cpp' '(?m)^\s*// Apply pending section-local chunkOffset from WorldRenderer\.\n' 4
RemoveAll 'Tessellator quad-index comment' 'client\render\Tessellator.cpp' '(?ms)^\s*// Meshes captured in quad mode keep 4 vertices per quad.*?at capture time\.\n' 1
RemoveAll 'Tessellator VBO copy comment' 'client\render\Tessellator.cpp' '(?ms)^\s*// Meshes carry their pose in the vertices \(applied at capture time\), so the\n\s*// VBO copy is always safe to draw directly\.\n' 1

$results | ForEach-Object { $_ }
Write-Output "TOTAL: $($results.Count) checks, $(( $results | Where-Object { $_ -like 'FAIL*' } ).Count) failures"
