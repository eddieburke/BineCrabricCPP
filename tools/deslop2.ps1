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
function Rep($desc, $state, $old, $new, [int]$expected = 1) {
  $count = ([regex]::Matches($state.text, [regex]::Escape($old))).Count
  if($count -ne $expected) { $script:report.Add("FAIL $desc (expected=$expected found=$count)"); return }
  $state.text = $state.text.Replace($old, $new)
  $script:report.Add("OK   $desc (x$count)")
}
function Remove($desc, $state, $block, [int]$expected = 1) {
  $count = ([regex]::Matches($state.text, [regex]::Escape($block))).Count
  if($count -ne $expected) { $script:report.Add("FAIL $desc (expected=$expected found=$count)"); return }
  $state.text = $state.text.Replace($block, '')
  $script:report.Add("OK   $desc (x$count)")
}

# ---------- A. Tessellator.hpp full rewrite ----------
$s = LoadNormalized 'client\render\Tessellator.hpp'
$newHpp = @'
#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "net/minecraft/client/render/BufferBuilder.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::render {
struct TessellatorMesh {
 std::vector<TessellatorVertex> vertices;
 int mode = 7;
 bool hasTexture = false;
 bool hasColor = false;
 bool hasNormals = false;
 TessellatorMesh() = default;
 TessellatorMesh(std::vector<TessellatorVertex> v, int m, bool ht, bool hc, bool hn)
     : vertices(std::move(v)), mode(m), hasTexture(ht), hasColor(hc), hasNormals(hn) {
 }
 TessellatorMesh(const TessellatorMesh& other);
 TessellatorMesh& operator=(const TessellatorMesh& other);
 TessellatorMesh(TessellatorMesh&& other) noexcept;
 TessellatorMesh& operator=(TessellatorMesh&& other) noexcept;
 ~TessellatorMesh();
 [[nodiscard]] bool empty() const noexcept {
  return vertices.empty();
 }
 [[nodiscard]] bool uploadToGpu();
 void freeGpuBuffer();

 private:
 friend class Tessellator;
 unsigned vbo_ = 0;
};
class Tessellator {
 public:
 static thread_local Tessellator INSTANCE;
 explicit Tessellator(std::size_t bufferSize = 4096);
 void startQuads();
 void start(int mode);
 void texture(double u, double v);
 void color(float r, float g, float b);
 void color(float r, float g, float b, float a);
 void color(int r, int g, int b);
 void color(int r, int g, int b, int a);
 void color(int rgb);
 void color(int rgb, int a);
 void light(float blockLight, float skyLight);
 void normal(float x, float y, float z);
 void blockData(double x,
                double y,
                double z,
                int emission,
                int blockLight = 15,
                int skyLight = 15,
                int blockId = 0,
                bool fluid = false,
                int metadata = 0);
 void translate(double x, double y, double z);
 void translate(float x, float y, float z);
 void vertex(double x, double y, double z, double u, double v);
 void vertex(double x, double y, double z);
 void draw();
 void beginBatch();
 void endBatch();
 class ScopedBatch {
  public:
  ScopedBatch() {
   Tessellator::INSTANCE.beginBatch();
  }
  ~ScopedBatch() {
   Tessellator::INSTANCE.endBatch();
  }
  ScopedBatch(const ScopedBatch&) = delete;
  ScopedBatch& operator=(const ScopedBatch&) = delete;
 };
 [[nodiscard]] TessellatorMesh takeMesh();
 static void drawMesh(const TessellatorMesh& mesh);
 [[nodiscard]] static int effectiveDrawMode(int mode) noexcept;
 void setCaptureOnly(bool captureOnly) noexcept {
  captureOnly_ = captureOnly;
 }
 [[nodiscard]] bool drawing() const noexcept {
  return drawing_;
 }

 private:
 static constexpr int kGlQuads = 7;
 static constexpr bool kTriangleMode = true;
 void expandQuadToTriangles();
 void finishQuad();
 void flush();
 void reset();
 void beginPart(int mode);
 int batchDepth_ = 0;
 net::minecraft::util::math::Matrix4f pose_{};
 bool poseValid_ = false;
 BufferBuilder<TessellatorVertex> builder_;
 bool drawing_ = false;
 bool hasTexture_ = false;
 bool hasColor_ = false;
 bool hasNormals_ = false;
 bool captureOnly_ = false;
 int addedVertexCount_ = 0;
 int mode_ = 7;
 float u_ = 0.0f;
 float v_ = 0.0f;
 double xOffset_ = 0.0;
 double yOffset_ = 0.0;
 double zOffset_ = 0.0;
 std::uint32_t currentColor_ = 0xFFFFFFFFU;
 std::int32_t currentNormal_ = 0;
 double blockCenterX_ = 0.0;
 double blockCenterY_ = 0.0;
 double blockCenterZ_ = 0.0;
 int blockEmission_ = 0;
 float blockLight_ = 15.0f;
 float skyLight_ = 15.0f;
 int blockId_ = 0;
 bool blockFluid_ = false;
 int blockMetadata_ = 0;
 bool hasBlockData_ = false;
};
} // namespace net::minecraft::client::render
'@
if($s.text.Contains('class ScopedBatch')) { $s.text = $newHpp; $script:report.Add('OK   Tessellator.hpp full rewrite') }
else { $script:report.Add('FAIL Tessellator.hpp (ScopedBatch missing - abort rewrite)') }
Save $s

# ---------- B. Tessellator.cpp comment removal ----------
$s = LoadNormalized 'client\render\Tessellator.cpp'
Remove 'Tess start() pose comment' $s "`n // Snapshot the pose for this batch AFTER reset(), which clears the previous`n // batch's snapshot. Mesh captures run with captureOnly and never draw; their`n // vertices stay model-local and the consumer supplies the transform, so the`n // flag keeps them out of this branch."
Remove 'Tess identity pose comment' $s "`n// An identity pose means the producer already emits pass-base-space positions`n// (terrain via chunkOffset, particles, the block outline). Skipping the transform`n// is worth a branch: it is the hot path for the largest vertex counts.`n"
Remove 'Tess defaults comment' $s "`n// Defaults in for vertices that never got their tangent/mid-tex filled: the`n// tangent is rotated by the draw pose when one is active so the fallback`n// direction matches the posed frame (the old end()-time bake did the same).`n"
Remove 'Tess double-cast comment' $s "`n // Add in double, then cast once - avoids float(world) + float(offset) collapse at far coords.`n"
Remove 'Tess iris parity comment' $s "`n // Iris parity: the pose lives in the vertices, not in modelViewMatrix (see`n // setDrawPose). Positions take the full transform; the face normal is rotated`n // here so the packed byte normal is already camera-relative. Tangents need no`n // extra work - finishQuad derives them from the (posed) corners.`n"
Remove 'Tess vaUV2 comment' $s "`n // vaUV2 is an unnormalized ushort2 of level*16 (0..240). Keeping the fraction`n // here is what lets a smooth-lit corner land between two lightmap texels, so`n // the GL_LINEAR filter reproduces the vanilla averaged-luminance value instead`n // of snapping to a whole level.`n"
Remove 'Tess last triangle comment' $s "`n // GL_TRIANGLES / GL_TRIANGLE_STRIP - fill last triangle.`n"
Remove 'Tess chunkOffset comment' $s "`n // Apply pending section-local chunkOffset from WorldRenderer.`n" 4
Remove 'Tess quad index comment' $s "`n // Meshes captured in quad mode keep 4 vertices per quad; draw them through the`n // shared quad index buffer instead of expanding to 6 vertices at capture time.`n"
Remove 'Tess VBO copy comment' $s "`n // Meshes carry their pose in the vertices (applied at capture time), so the`n // VBO copy is always safe to draw directly.`n"
Save $s

# ---------- C. Manager.cpp deslop ----------
$s = LoadNormalized 'client\render\pipeline\Manager.cpp'
function RepRx($desc, $state, $pattern, $replacement, [int]$expected = 1) {
  $rx = [regex]::new($pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
  $n = $rx.Matches($state.text).Count
  if($n -ne $expected) { $script:report.Add("FAIL $desc (expected=$expected found=$n)"); return }
  $state.text = $rx.Replace($state.text, $replacement)
  $script:report.Add("OK   $desc (x$n)")
}
Remove 'Manager log var decl' $s " PackCompiler::LogFnLevel log = packLogFn(*this);`n"
RepRx 'Manager pending call' $s 'prewarmStep\(\*pending, kPrewarmMaxProgramsPerFrame, kPrewarmTimeBudgetMs, log\)' 'prewarmStep(*pending, logFn())'
RepRx 'Manager staged call (log var)' $s 'prewarmStep\(\*stagedPack_, kPrewarmMaxProgramsPerFrame, kPrewarmTimeBudgetMs, log\)' 'prewarmStep(*stagedPack_, logFn())'
RepRx 'Manager staged calls (packLogFn)' $s 'prewarmStep\(\*stagedPack_, kPrewarmMaxProgramsPerFrame, kPrewarmTimeBudgetMs,\s*\n\s*packLogFn\(\*this\)\)' 'prewarmStep(*stagedPack_, logFn())' 2
RepRx 'Manager pending call (packLogFn)' $s 'prewarmStep\(\*pack, kPrewarmMaxProgramsPerFrame, kPrewarmTimeBudgetMs,\s*\n\s*packLogFn\(\*this\)\)' 'prewarmStep(*pack, logFn())'
Rep 'Manager logFn impl' $s "void PackManager::logOnce(PackInstance& pack, const std::string& message) const {" "PackCompiler::LogFnLevel PackManager::logFn() {`n return [this](PackInstance& p, const std::string& message, ::net::minecraft::util::logging::LogLevel) {`n  logOnce(p, message);`n };`n}`nvoid PackManager::logOnce(PackInstance& pack, const std::string& message) const {"
Rep 'Manager setSettings queue clear' $s "if(!target->rebuildRuntime(customError)) logOnce(*target, customError);`n  }" "if(!target->rebuildRuntime(customError)) logOnce(*target, customError);`n   target->prewarmQueue.clear();`n  }"
Remove 'Manager profile baseline comment' $s "`n // The selected profile's values become the baseline, then the explicit values in`n // this call win over them - Iris's options screen applies a profile by setting the`n // option values, which the user can then override individually. There is no`n // profile logic anywhere else: the properties preprocessor and the GLSL option`n // rewrite both read the settings map, so one merged map keeps them agreeing.`n"
Remove 'Manager profile preset comment' $s "`n // Selecting a profile applies the whole preset (Iris behaviour): reset to the`n // pack's shipped defaults, then lay the selected profile's values over them.`n // Individual options changed after the fact win via the explicit values below.`n"
Remove 'Manager staged parse comment' $s "`n  // Stage a fully re-parsed pack carrying the merged settings, so the`n  // properties-derived flags (shadowEntities and friends) and the GLSL agree on`n  // every option; swap it in when its programs are ready.`n"
Remove 'Manager re-run parse comment' $s "`n // Re-run the parse with the CURRENT settings instead of reusing the load-time`n // definition: option changes (including profile selection) must reach the`n // properties-derived state (shadowEntities/shadowPlayer/shadowBlockEntities, the`n // LL_CAPACITY-conditional bufferObject sizes, bufferBlends) and the GLSL alike,`n // or the engine flag and the compiled shaders quietly disagree - which is how`n // `"SM_ENTITY on`" recompiled the pack and still rendered no entity shadows.`n // A failed re-parse keeps the copied definition above, so the pack degrades to`n // its previous behaviour instead of going dark.`n"
Remove 'Manager vanilla-pack comment' $s "`n  // The vanilla pack ships inside the executable (EmbeddedVanillaPack.cpp); the`n  // on-disk folder is only consulted when present, so it can be tweaked in place.`n"
Remove 'Manager empty-id comment' $s "`n  // The vanilla definition carries empty id maps, so the lookups below miss and the`n  // caller's fallback is returned without a pack-presence branch.`n"
Save $s

# ---------- D. Pipeline.cpp ----------
$s = LoadNormalized 'client\render\pipeline\Pipeline.cpp'
Rep 'Pipeline prewarmStep single-arg' $s "PackCompiler::prewarmStep(*pack, 8, 12,`n                            [this](PackInstance& p, const std::string& message, LogLevel level) {`n                             logOnce(p, message, level);`n                            });" "PackCompiler::prewarmStep(*pack,`n                            [this](PackInstance& p, const std::string& message, LogLevel level) {`n                             logOnce(p, message, level);`n                            });"
Remove 'Pipeline budgeted comment' $s "`n  // Budgeted: a cold pack spreads across frames instead of stalling one tick.`n"
Save $s

# ---------- E. Compiler.cpp ----------
$s = LoadNormalized 'client\render\shaders\Compiler.cpp'
Remove 'Compiler draw buffers comment' $s "`n// Draw buffer indices live in the prepared fragment source, which is expensive to`n// rebuild. Stash them under the cache key while we have them so a later link can`n// apply them without preparing the program a second time.`n"
Remove 'Compiler embedded comment' $s "`n// Embedded vanilla sources are baked fully resolved at configure time`n// (gen-embedded-vanilla-pack.cmake expands #include and strips comments), so`n// the include machinery never runs for the built-in pack.`n"
Save $s

# ---------- F. Instance.hpp ----------
$s = LoadNormalized 'client\render\pipeline\Instance.hpp'
Remove 'Instance cache key comment' $s "`n // What prewarm already worked out for each program, so the pass that links the`n // finished binaries does not have to re-run source preparation to rediscover it.`n // program name -> ProgramCache key.`n"
Remove 'Instance draw buffers comment' $s "`n // ProgramCache key -> draw buffer indices parsed out of the prepared fragment.`n"
Remove 'Instance compiler owner comment' $s "`n // Owned by the owner of this instance's render pipeline (Minecraft via`n // GameRenderer/PackManager); set before resetPrograms() runs. Compilation and`n // the disk cache live on it so every pack shares one cache directory.`n"
Save $s

# ---------- G. CoreGlslTransformer.cpp ----------
$s = LoadNormalized 'client\render\shaders\CoreGlslTransformer.cpp'
Remove 'Transformer one-mask comment' $s "`n // One mask serves the whole declaration batch: appendMissingDeclarations and`n // patchMultiTexCoord3 only read the source (patchMultiTexCoord3's token rewrite`n // re-scans internally after its mutation).`n"
Remove 'Transformer gl_FragData comment' $s "`n// gl_FragData[N] writes DRAW BUFFER N, which the program's RENDERTARGETS directive maps`n// to a colortex. If that colortex has an integer format the output must be declared`n// uvec4/ivec4 - `out vec4` against an integer attachment is undefined, and the write is`n// silently dropped rather than failing to link. Packs that declare their own`n// `layout(location = N) out uvec4` were always fine; this is the legacy path only.`n"
Remove 'Transformer safe-to-parse comment' $s "`n // Safe to parse fatally: Pipeline builds the scene targets from these same strings.`n"
Remove 'Transformer bias overload comment' $s "`n // The bias overload of texture() is fragment-stage-only; ANGLE rejects its`n // mere presence in compute/vertex programs, so the bias is dropped there.`n"
Remove 'Transformer clamp bounds comment' $s "`n// Strict GLSL compilers reject clamp/min/max calls whose bounds mix integer and`n// float literals (`clamp(x + 1.6, 0.6, 1)`), and packs written for lenient drivers`n// ship them. Rewrite only the unambiguous cases: a bound that is a bare integer`n// literal next to a float-typed sibling, or both bounds integer next to a`n// float-typed first argument. All-integer calls (valid int clamps like`n// `clamp01(int)` / `min(i, 5)`) are left untouched.`n"
Remove 'Transformer fog globals comment' $s "`n// gl_Fog.start/.end/.scale/.color were fixed-function builtins removed in GLSL 1.40.`n// Packs that still read them (usually for fog blending in compute or post stages)`n// get them backed by the engine's live fog uniforms instead.`n"
Save $s

# ---------- H. WorldChunks.cpp ----------
$s = LoadNormalized 'world\WorldChunks.cpp'
Remove 'WorldChunks sky-color comment' $s "`n // Java uses the raw sky-color temperature sampler here, not transformed biome climate.`n"
Save $s

$report | ForEach-Object { $_ }
Write-Output "TOTAL: $($report.Count) items, $(( $report | Where-Object { $_ -like 'FAIL*' } ).Count) failures"
