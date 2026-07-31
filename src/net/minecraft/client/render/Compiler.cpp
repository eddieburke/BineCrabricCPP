#include "net/minecraft/client/render/shaderpack/ShaderPackCompiler.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackGl.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackInstance.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackCatalog.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackLoader.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
namespace net::minecraft::client::render::shaderpack {
namespace {
std::string makeCacheKey(const std::string& programName, const std::string& vertex, const std::string& fragment) {
 return programName + "|" + vertex + "|" + fragment;
}
std::string makeComputeCacheKey(const std::string& programName, const std::string& compute) {
 return programName + "|" + compute;
}
}
std::string ShaderPackCompiler::readText(const ShaderPackInstance& pack, const std::string& path) {
 const std::filesystem::path normalized = std::filesystem::path(path).lexically_normal();
 if(normalized.empty() || normalized.is_absolute() ||
    std::any_of(
        normalized.begin(), normalized.end(), [](const std::filesystem::path& part) { return part == ".."; })) {
  return {};
 }
 if(pack.directory) {
  return ShaderPackCatalog::readFile(pack.path / normalized);
 }
 if(pack.zip == nullptr) {
  return {};
 }
 const std::vector<std::uint8_t> bytes = pack.zip->getResource(normalized.generic_string());
 return std::string(bytes.begin(), bytes.end());
}
const std::string& ShaderPackCompiler::cachedText(ShaderPackInstance& pack, const std::string& path) {
 const auto found = pack.sourceCache.find(path);
 if(found != pack.sourceCache.end()) {
  return found->second;
 }
 return pack.sourceCache.emplace(path, readText(pack, path)).first->second;
}
std::string ShaderPackCompiler::resolveIncludes(ShaderPackInstance& pack, const std::string& path) {
 std::string dimPrefix;
 if(path.rfind("shaders/", 0) == 0) {
  const std::size_t slash = path.find('/', 8);
  if(slash != std::string::npos) {
   dimPrefix = path.substr(0, slash + 1);
  }
 }
 return glutil::resolveShaderIncludes(
     [&](std::string_view current) {
      std::string target(current);
      if(!dimPrefix.empty() && target.rfind("shaders/", 0) == 0 && target.rfind(dimPrefix, 0) != 0) {
       const std::string dimTarget = dimPrefix + target.substr(8);
       std::string source = cachedText(pack, dimTarget);
       if(!source.empty()) {
        return ShaderPackLoader::rewriteOptions(source, pack.sourceOptions, pack.settings);
       }
      }
      std::string source = cachedText(pack, target);
      return ShaderPackLoader::rewriteOptions(source, pack.sourceOptions, pack.settings);
     },
     path,
     true);
}
gl::ShaderProgram* ShaderPackCompiler::compile(ShaderPackInstance& pack, const std::string& programName,
                                               const LogFnLevel& logOnce) {
 return compile(pack, programName, [&](ShaderPackInstance& p, const std::string& message) {
  logOnce(p, message, ::net::minecraft::util::logging::LogLevel::Info);
 });
}
gl::ShaderProgram* ShaderPackCompiler::compile(ShaderPackInstance& pack, const std::string& programName,
                                               const LogFn& logOnce) {
 if(!pack.summary.valid || pack.programs == nullptr) {
  return nullptr;
 }
 for(const std::string& feature : pack.definition.requiredFeatures) {
  if(!glutil::featureSupported(feature)) {
   logOnce(pack, "required feature '" + feature + "' is unavailable");
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
 const ShaderProgramSource& spec = found->second;
 if(spec.stage == "compute") {
  if(cachedText(pack, spec.compute).empty()) {
   logOnce(pack, "program '" + programName + "' missing shader source (" + spec.compute + ")");
   return nullptr;
  }
  const std::string computeIncluded = resolveIncludes(pack, spec.compute);
  const std::string computePreamble = glutil::versionPreamble(pack.definition, computeIncluded, true);
  const std::string compute = glutil::prepareSource(
      programName, glutil::ShaderStage::Other, pack.definition, computeIncluded, computePreamble);
  const std::string cacheKey = makeComputeCacheKey(programName, spec.compute);
  gl::ShaderProgram* program = pack.programs->getFromComputeSource(cacheKey, compute, computePreamble);
  if(program != nullptr) {
   pack.compiledPrograms.emplace(programName, program);
   return program;
  }
  const std::string& error = pack.programs->compileError(cacheKey);
  logOnce(pack,
          "program '" + programName + "' (" + spec.compute + ") failed to compile: " +
              (error.empty() ? "no driver info log" : error));
  return nullptr;
 }
 if(cachedText(pack, spec.fragment).empty()) {
  logOnce(pack,
          "program '" + programName + "' missing shader source (" + spec.vertex + ", " + spec.fragment + ")");
  return nullptr;
 }
 const bool compositeProgram = glutil::isCompositeStyleProgramName(programName);
 if(!compositeProgram && cachedText(pack, spec.vertex).empty()) {
  logOnce(pack,
          "program '" + programName + "' missing shader source (" + spec.vertex + ", " + spec.fragment + ")");
  return nullptr;
 }
 const std::string vertexIncluded =
     spec.vertex.empty() && compositeProgram ? std::string(glutil::defaultCompositeVertexShader())
                                               : resolveIncludes(pack, spec.vertex);
 if(vertexIncluded.empty()) {
  logOnce(pack, "program '" + programName + "' missing vertex stage");
  return nullptr;
 }
 const std::string fragmentIncluded = resolveIncludes(pack, spec.fragment);
 const std::string geometryIncluded = spec.geometry.empty() ? std::string{} : resolveIncludes(pack, spec.geometry);
 const std::string tessControlIncluded =
     spec.tessControl.empty() ? std::string{} : resolveIncludes(pack, spec.tessControl);
 const std::string tessEvaluationIncluded =
     spec.tessEvaluation.empty() ? std::string{} : resolveIncludes(pack, spec.tessEvaluation);

 const std::string preamble = glutil::versionPreamble(pack.definition, vertexIncluded);
 const auto prepare = [&](glutil::ShaderStage stage, const std::string& source) {
  return source.empty()
             ? std::string{}
             : glutil::prepareSource(programName, stage, pack.definition, source, preamble);
 };
 const std::string vertex = prepare(glutil::ShaderStage::Vertex, vertexIncluded);
 const std::string fragment = prepare(glutil::ShaderStage::Fragment, fragmentIncluded);
 const std::string geometry = prepare(glutil::ShaderStage::Other, geometryIncluded);
 const std::string tessControl = prepare(glutil::ShaderStage::Other, tessControlIncluded);
 const std::string tessEvaluation = prepare(glutil::ShaderStage::Other, tessEvaluationIncluded);
 const std::string cacheKey = makeCacheKey(programName,
                                           spec.vertex + "|" + spec.geometry + "|" + spec.tessControl + "|" +
                                               spec.tessEvaluation,
                                           spec.fragment);
 gl::ShaderProgram* program = pack.programs->getFromSource(
     cacheKey, vertex, fragment, preamble, geometry, tessControl, tessEvaluation);
 if(program != nullptr) {
  std::vector<int> targets = glutil::parseRenderTargetIndices(fragment);
  if(targets.empty()) {
   targets = glutil::defaultRenderTargetIndices();
  }
  program->setDrawBufferColortexIndices(targets);
  pack.compiledPrograms.emplace(programName, program);
  return program;
 }
 const std::string& error = pack.programs->compileError(cacheKey);
 logOnce(pack,
         "program '" + programName + "' (" + spec.vertex + " + " + spec.fragment +
             ") failed to compile: " + (error.empty() ? "no driver info log" : error));
 return nullptr;
}
}
