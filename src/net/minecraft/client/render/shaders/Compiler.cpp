#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
namespace net::minecraft::client::render {
namespace diagnostics = net::minecraft::client::diagnostics;
namespace {
std::string makeCacheKey(const std::string& programName, const std::string& vertex, const std::string& fragment) {
 return programName + "|" + vertex + "|" + fragment;
}
std::string makeComputeCacheKey(const std::string& programName, const std::string& compute) {
 return programName + "|" + compute;
}
ShaderTransformContext transformContextFor(const std::string& programName,
                                           bool hasGeometry,
                                           bool hasTessellation) {
 std::string_view name = programName;
 if(name.starts_with("clrwl_")) name.remove_prefix(6);
 const bool overlay = name.starts_with("gbuffers_entities") || name.starts_with("gbuffers_block") ||
                      name.starts_with("gbuffers_hand") || name == "gbuffers_item" ||
                      name == "gbuffers_lightning" || name == "gbuffers_spidereyes" ||
                      name == "gbuffers_armor_glint" || name == "gbuffers_beaconbeam" ||
                      name == "shadow_entities" || name == "shadow_block";
 const bool entityId = overlay || name == "gbuffers_text" || name == "shadow";
 return ShaderTransformContext{overlay, entityId, hasGeometry, hasTessellation};
}
struct PreparedProgram {
 bool compute = false;
 std::string cacheKey;
 std::string preamble;
 std::string vertex;
 std::string fragment;
 std::string geometry;
 std::string tessControl;
 std::string tessEvaluation;
};
// Draw buffer indices live in the prepared fragment source, which is expensive to
// rebuild. Stash them under the cache key while we have them so a later link can
// apply them without preparing the program a second time.
void rememberDrawBuffers(PackInstance& pack, const PreparedProgram& prepared) {
 std::vector<int> targets = parseRenderTargetIndices(prepared.fragment);
 if(targets.empty()) {
  targets = defaultRenderTargetIndices();
 }
 pack.programDrawBuffers.insert_or_assign(prepared.cacheKey, std::move(targets));
}
bool prepareProgram(PackInstance& pack, const std::string& programName, const PackProgramSource& spec,
                    const PackCompiler::LogFnLevel& logOnce, PreparedProgram& out) {
 diagnostics::WorkSpan span("shaderpack.prepare");
 const auto log = [&](const std::string& message) {
  logOnce(pack, message, ::net::minecraft::util::logging::LogLevel::Info);
 };
 if(!spec.compute.empty()) {
  if(PackCompiler::cachedText(pack, spec.compute).empty()) {
   log("program '" + programName + "' missing shader source (" + spec.compute + ")");
   return false;
  }
  const std::string computeIncluded = PackCompiler::resolveIncludes(pack, spec.compute);
  out.compute = true;
  out.cacheKey = makeComputeCacheKey(programName, spec.compute);
  out.preamble = versionPreamble(pack.definition, computeIncluded, true);
  out.vertex = prepareSource(programName, ShaderStage::Compute, pack.definition, computeIncluded);
  return true;
 }
 if(PackCompiler::cachedText(pack, spec.fragment).empty()) {
  log("program '" + programName + "' missing shader source (" + spec.vertex + ", " + spec.fragment + ")");
  return false;
 }
 const bool compositeProgram = isCompositeStageName(programName);
 const std::string vertexIncluded =
     spec.vertex.empty()
         ? compositeProgram ? defaultCompositeVertexShader() : defaultRasterVertexShader()
         : PackCompiler::resolveIncludes(pack, spec.vertex);
 if(vertexIncluded.empty()) {
  log("program '" + programName + "' missing vertex stage");
  return false;
 }
 const std::string fragmentIncluded = PackCompiler::resolveIncludes(pack, spec.fragment);
 const std::string geometryIncluded =
     spec.geometry.empty() ? std::string{} : PackCompiler::resolveIncludes(pack, spec.geometry);
 const std::string tessControlIncluded =
     spec.tessControl.empty() ? std::string{} : PackCompiler::resolveIncludes(pack, spec.tessControl);
 const std::string tessEvaluationIncluded =
     spec.tessEvaluation.empty() ? std::string{} : PackCompiler::resolveIncludes(pack, spec.tessEvaluation);
 out.compute = false;
 const bool hasTessellation = !tessControlIncluded.empty() || !tessEvaluationIncluded.empty();
 out.preamble = versionPreambleForStages(
     pack.definition,
     {vertexIncluded, fragmentIncluded, geometryIncluded, tessControlIncluded, tessEvaluationIncluded},
     hasTessellation ? 400 : 330);
 const ShaderTransformContext transformContext =
     transformContextFor(programName, !geometryIncluded.empty(), hasTessellation);
 const auto prepare = [&](ShaderStage stage, const std::string& source) {
  return source.empty()
             ? std::string{}
             : prepareSource(programName, stage, pack.definition, source, transformContext);
 };
 out.vertex = prepare(ShaderStage::Vertex, vertexIncluded);
 out.fragment = prepare(ShaderStage::Fragment, fragmentIncluded);
 out.geometry = prepare(ShaderStage::Geometry, geometryIncluded);
 out.tessControl = prepare(ShaderStage::TessControl, tessControlIncluded);
 out.tessEvaluation = prepare(ShaderStage::TessEvaluation, tessEvaluationIncluded);
 out.cacheKey = makeCacheKey(programName,
                             spec.vertex + "|" + spec.geometry + "|" + spec.tessControl + "|" + spec.tessEvaluation,
                             spec.fragment);
 return true;
}
} // namespace
std::string PackCompiler::readText(const PackInstance& pack, const std::string& path) {
 diagnostics::WorkSpan span("shaderpack.read");
 const std::filesystem::path normalized = std::filesystem::path(path).lexically_normal();
 if(normalized.empty() || normalized.is_absolute() ||
    std::any_of(
        normalized.begin(), normalized.end(), [](const std::filesystem::path& part) { return part == ".."; })) {
  return {};
 }
  if(pack.embedded) {
   return VanillaPackEmbed::get(normalized.generic_string());
  }
  if(pack.directory) {
  return PackCatalog::readFile(pack.path / normalized);
 }
 if(pack.zip == nullptr) {
  return {};
 }
 const std::vector<std::uint8_t> bytes = pack.zip->getResource(normalized.generic_string());
 return std::string(bytes.begin(), bytes.end());
}
const std::string& PackCompiler::cachedText(PackInstance& pack, const std::string& path) {
 const auto found = pack.sourceCache.find(path);
 if(found != pack.sourceCache.end()) {
  return found->second;
 }
 return pack.sourceCache.emplace(path, readText(pack, path)).first->second;
}
std::string PackCompiler::resolveIncludes(PackInstance& pack, const std::string& path) {
 if(pack.embedded) {
  // Embedded vanilla sources are baked fully resolved at configure time
  // (gen-embedded-vanilla-pack.cmake expands #include and strips comments), so
  // the include machinery never runs for the built-in pack.
  return cachedText(pack, path);
 }
 if(const auto cached = pack.resolvedSourceCache.find(path); cached != pack.resolvedSourceCache.end()) {
  return cached->second;
 }
 diagnostics::WorkSpan span("shaderpack.includes");
 std::string dimPrefix;
 if(path.rfind("shaders/", 0) == 0) {
  const std::size_t slash = path.find('/', 8);
  if(slash != std::string::npos) {
   dimPrefix = path.substr(0, slash + 1);
  }
 }
 std::string resolved = resolveShaderIncludes(
     [&](std::string_view current) {
      std::string target(current);
      if(!dimPrefix.empty() && target.rfind("shaders/", 0) == 0 && target.rfind(dimPrefix, 0) != 0) {
       const std::string dimTarget = dimPrefix + target.substr(8);
       std::string source = cachedText(pack, dimTarget);
       if(!source.empty()) {
        return PackLoader::rewriteOptions(source, pack.sourceOptions, pack.settings);
       }
      }
      std::string source = cachedText(pack, target);
      return PackLoader::rewriteOptions(source, pack.sourceOptions, pack.settings);
     },
     path,
     true);
 pack.resolvedSourceCache.emplace(path, resolved);
 return resolved;
}
gl::ShaderProgram* PackCompiler::compile(PackInstance& pack, const std::string& programName,
                                         const LogFnLevel& logOnce) {
 if(!pack.summary.valid || pack.programs == nullptr) {
  return nullptr;
 }
 for(const std::string& feature : pack.definition.requiredFeatures) {
  if(!featureSupported(feature)) {
   logOnce(pack, "required feature '" + feature + "' is unavailable",
           ::net::minecraft::util::logging::LogLevel::Info);
   return nullptr;
  }
 }
 const auto compiled = pack.compiledPrograms.find(programName);
 if(compiled != pack.compiledPrograms.end()) {
  return compiled->second;
 }
 const auto found = pack.definition.programs.find(programName);
 if(found == pack.definition.programs.end()) {
  return nullptr;
 }
 // Prewarm already prepared this program and recorded its cache key. Preparing the
 // sources again only to rediscover that key costs as much as the original pass —
 // for a big pack that is a multi-second stall on the frame that finishes loading.
  if(const auto keyed = pack.programCacheKeys.find(programName); keyed != pack.programCacheKeys.end()) {
   if(gl::ShaderProgram* linked = pack.programs->find(keyed->second)) {
    if(const auto targets = pack.programDrawBuffers.find(keyed->second);
       targets != pack.programDrawBuffers.end()) {
     linked->setDrawBufferColortexIndices(targets->second);
    }
    pack.compiledPrograms.emplace(programName, linked);
    return linked;
   }
  }
 PreparedProgram prepared;
 if(!prepareProgram(pack, programName, found->second, logOnce, prepared)) {
  return nullptr;
 }
 pack.programCacheKeys.insert_or_assign(programName, prepared.cacheKey);
  if(prepared.compute) {
   gl::ShaderProgram* program =
       pack.programs->getFromComputeSource(prepared.cacheKey, prepared.vertex, prepared.preamble);
   if(program != nullptr) {
    pack.compiledPrograms.emplace(programName, program);
    return program;
   }
   const std::string& error = pack.programs->compileError(prepared.cacheKey);
  logOnce(pack,
          "program '" + programName + "' (" + found->second.compute + ") failed to compile: " +
              (error.empty() ? "no driver info log" : error),
          ::net::minecraft::util::logging::LogLevel::Info);
  return nullptr;
 }
 gl::ShaderProgram* program =
     pack.programs->getFromSource(prepared.cacheKey, prepared.vertex, prepared.fragment, prepared.preamble,
                                  prepared.geometry, prepared.tessControl, prepared.tessEvaluation);
 if(program != nullptr) {
  rememberDrawBuffers(pack, prepared);
  program->setDrawBufferColortexIndices(pack.programDrawBuffers.at(prepared.cacheKey));
  pack.compiledPrograms.emplace(programName, program);
  return program;
 }
  rememberDrawBuffers(pack, prepared);
  const std::string& error = pack.programs->compileError(prepared.cacheKey);
 logOnce(pack,
         "program '" + programName + "' (" + found->second.vertex + " + " + found->second.fragment +
             ") failed to compile: " + (error.empty() ? "no driver info log" : error),
         ::net::minecraft::util::logging::LogLevel::Info);
 return nullptr;
}
void PackCompiler::prewarm(PackInstance& pack, const LogFnLevel& logOnce) {
 if(!pack.summary.valid || pack.programs == nullptr || pack.programState != PackProgramState::Cold) {
  return;
 }
 diagnostics::WorkSpan span("shaderpack.prewarm");
 pack.programState = PackProgramState::Submitted;
 for(const std::string& feature : pack.definition.requiredFeatures) {
  if(!featureSupported(feature)) {
   pack.programState = PackProgramState::Failed;
   return;
  }
 }
 for(const auto& [name, spec] : pack.definition.programs) {
  if(!isProgramEnabledCached(pack.definition, pack.settings, name, pack.programEnabledCache)) {
   continue;
  }
   PreparedProgram prepared;
   if(!prepareProgram(pack, name, spec, logOnce, prepared)) {
    continue;
   }
   if(prepared.compute) {
    pack.programs->getFromComputeSource(prepared.cacheKey, prepared.vertex, prepared.preamble);
   } else {
    pack.programs->getFromSource(prepared.cacheKey, prepared.vertex, prepared.fragment, prepared.preamble,
                                 prepared.geometry, prepared.tessControl, prepared.tessEvaluation);
    rememberDrawBuffers(pack, prepared);
   }
   pack.programCacheKeys.insert_or_assign(name, prepared.cacheKey);
  }
 }
bool PackCompiler::validate(PackInstance& pack, const LogFnLevel& logOnce) {
 if(!pack.summary.valid || pack.programs == nullptr || pack.programState == PackProgramState::Cold) {
  return false;
 }
 if(pack.programState == PackProgramState::Ready) return true;
 if(pack.programState == PackProgramState::Failed) return false;
 bool ready = true;
 for(const auto& [name, spec] : pack.definition.programs) {
  (void)spec;
  if(!isProgramEnabledCached(pack.definition, pack.settings, name, pack.programEnabledCache)) continue;
  if(compile(pack, name, logOnce) == nullptr) ready = false;
 }
 pack.programState = ready ? PackProgramState::Ready : PackProgramState::Failed;
 return ready;
}
} // namespace net::minecraft::client::render
