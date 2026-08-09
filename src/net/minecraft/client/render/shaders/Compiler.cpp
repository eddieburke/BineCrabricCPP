#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/render/GlState.hpp"
#include "net/minecraft/client/render/shaders/IncludeResolver.hpp"
#include "net/minecraft/client/render/shaders/PassIndex.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaders/ShaderDump.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::render {
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
 ShaderTransformContext context;
};
void rememberDrawBuffers(PackInstance& pack, const PreparedProgram& prepared) {
 std::vector<int> targets = parseRenderTargetIndices(prepared.fragment);
 if(targets.empty()) {
  targets = defaultRenderTargetIndices();
 }
 pack.programDrawBuffers.insert_or_assign(prepared.cacheKey, std::move(targets));
}
bool prepareProgram(PackInstance& pack, const std::string& programName, const PackProgramSource& spec,
                    const PackCompiler::LogFnLevel& logOnce, PreparedProgram& out) {
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
  out.vertex = computeIncluded;
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
 out.vertex = vertexIncluded;
 out.fragment = fragmentIncluded;
 out.geometry = geometryIncluded;
 out.tessControl = tessControlIncluded;
 out.tessEvaluation = tessEvaluationIncluded;
 out.context = transformContext;
 out.cacheKey = makeCacheKey(programName,
                             spec.vertex + "|" + spec.geometry + "|" + spec.tessControl + "|" + spec.tessEvaluation,
                             spec.fragment);
 return true;
}
std::string transformationSalt(const PackDefinition& definition,
                               const std::string& programName,
                               const ShaderTransformContext& context) {
 std::string salt = render::vertex_abi::abiSaltString() + "|shader-transform-5|" + programName;
 salt += context.entityOverlay ? "|overlay" : "|no-overlay";
 salt += context.entityId ? "|entity-id" : "|no-entity-id";
 salt += context.hasGeometry ? "|geometry" : "|no-geometry";
 salt += context.hasTessellation ? "|tessellation" : "|no-tessellation";
 std::vector<std::pair<std::string, std::string>> formats;
 formats.reserve(definition.targets.size());
 for(const auto& [name, target] : definition.targets) formats.emplace_back(name, target.format);
 std::sort(formats.begin(), formats.end());
 for(const auto& [name, format] : formats) salt += '|' + name + '=' + format;
 return salt;
}
std::uint64_t preparedContentHash(const PackDefinition& definition,
                                  const std::string& programName,
                                  const PreparedProgram& program) {
 return gl::ShaderProgram::contentHash(
     program.compute, program.preamble, program.vertex, program.fragment, program.geometry,
     program.tessControl, program.tessEvaluation,
     transformationSalt(definition, programName, program.context));
}
void transformProgram(PackInstance& pack, const std::string& programName, PreparedProgram& program) {
 const auto prepare = [&](ShaderStage stage, const std::string& source) {
  if(source.empty()) return std::string{};
  const std::string key = programName + ":" + std::to_string(static_cast<int>(stage));
  if(auto it = pack.preparedSourceCache.find(key); it != pack.preparedSourceCache.end()) return it->second;
  std::string result = prepareSource(programName, stage, pack.definition, source, program.context);
  pack.preparedSourceCache.emplace(key, result);
  return result;
 };
 if(program.compute) {
  program.vertex = prepare(ShaderStage::Compute, program.vertex);
  return;
 }
 program.vertex = prepare(ShaderStage::Vertex, program.vertex);
 program.fragment = prepare(ShaderStage::Fragment, program.fragment);
 program.geometry = prepare(ShaderStage::Geometry, program.geometry);
 program.tessControl = prepare(ShaderStage::TessControl, program.tessControl);
 program.tessEvaluation = prepare(ShaderStage::TessEvaluation, program.tessEvaluation);
}
} // namespace
std::string PackCompiler::readText(const PackInstance& pack, const std::string& path) {
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
   return cachedText(pack, path);
  }
  std::string dimPrefix;
 if(path.rfind("shaders/", 0) == 0) {
  const std::size_t slash = path.find('/', 8);
  if(slash != std::string::npos) {
   dimPrefix = path.substr(0, slash + 1);
  }
 }
 std::unordered_map<std::string, std::string>& memo = pack.includedSourceCache[dimPrefix];
 if(const auto cached = memo.find(path); cached != memo.end()) {
  return cached->second;
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
     true,
     memo);
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
   pack.compiledPrograms.insert_or_assign(programName, PackInstance::CompiledEntry{nullptr, true});
   return nullptr;
  }
 }
 const auto compiled = pack.compiledPrograms.find(programName);
 if(compiled != pack.compiledPrograms.end()) {
  return compiled->second.failed ? nullptr : compiled->second.program;
 }
 const auto found = pack.definition.programs.find(programName);
 if(found == pack.definition.programs.end()) {
  pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{nullptr, true});
  return nullptr;
 }
 PreparedProgram prepared;
 if(!prepareProgram(pack, programName, found->second, logOnce, prepared)) {
  pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{nullptr, true});
  return nullptr;
 }
 const std::uint64_t contentHash = preparedContentHash(pack.definition, programName, prepared);
 if(gl::ShaderProgram* program = pack.programs->loadBinary(prepared.cacheKey, contentHash)) {
  if(!prepared.compute) {
   rememberDrawBuffers(pack, prepared);
   program->setDrawBufferColortexIndices(pack.programDrawBuffers.at(prepared.cacheKey));
  }
  pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{program, false});
  return program;
 }
 transformProgram(pack, programName, prepared);
 if(prepared.compute) {
  gl::ShaderProgram* program =
      pack.programs->compileFromComputeSource(prepared.cacheKey, contentHash, prepared.vertex, prepared.preamble);
  if(program != nullptr) {
   pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{program, false});
   return program;
  }
  const std::string& error = pack.programs->compileError(prepared.cacheKey);
  logOnce(pack,
          "program '" + programName + "' (" + found->second.compute + ") failed to compile: " +
              (error.empty() ? "no driver info log" : error) +
              dumpFailedProgram(programName, prepared.preamble, {{".csh", &prepared.vertex}}, error),
          ::net::minecraft::util::logging::LogLevel::Info);
  pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{nullptr, true});
  return nullptr;
 }
 gl::ShaderProgram* program =
     pack.programs->compileFromSource(prepared.cacheKey, contentHash, prepared.vertex, prepared.fragment,
                                      prepared.preamble, prepared.geometry, prepared.tessControl,
                                      prepared.tessEvaluation);
 if(program != nullptr) {
  rememberDrawBuffers(pack, prepared);
  program->setDrawBufferColortexIndices(pack.programDrawBuffers.at(prepared.cacheKey));
  pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{program, false});
  return program;
 }
 rememberDrawBuffers(pack, prepared);
 const std::string& error = pack.programs->compileError(prepared.cacheKey);
 logOnce(pack,
         "program '" + programName + "' (" + found->second.vertex + " + " + found->second.fragment +
             ") failed to compile: " + (error.empty() ? "no driver info log" : error) +
             dumpFailedProgram(programName, prepared.preamble,
                               {{".vsh", &prepared.vertex},
                                {".fsh", &prepared.fragment},
                                {".gsh", &prepared.geometry},
                                {".tcs", &prepared.tessControl},
                                {".tes", &prepared.tessEvaluation}},
                               error),
         ::net::minecraft::util::logging::LogLevel::Info);
 pack.compiledPrograms.emplace(programName, PackInstance::CompiledEntry{nullptr, true});
 return nullptr;
}
} // namespace net::minecraft::client::render
