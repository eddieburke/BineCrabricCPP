#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include <utility>
namespace net::minecraft::client::gl {
namespace {
enum class CacheBinaryOutcome {
  Loaded,
  Failed,
  Unsupported,
};
CacheBinaryOutcome loadBinary(ShaderProgram& program,
                              ShaderCompileRequest request,
                              ShaderCompileResult& resultOut,
                              ShaderCompileService& compiler) {
 ShaderCompileResult result = compiler.compileBlocking(std::move(request));
 resultOut = std::move(result);
 if(!resultOut.ok) {
  return resultOut.binaryUnsupported ? CacheBinaryOutcome::Unsupported : CacheBinaryOutcome::Failed;
 }
 if(program.loadFromBinary(resultOut.binary)) {
  return CacheBinaryOutcome::Loaded;
 }
 compiler.invalidateDiskEntry(resultOut.contentHash);
 return CacheBinaryOutcome::Unsupported;
}
ShaderCompileRequest buildRequest(bool compute,
                                  const std::string& vertexSource,
                                  const std::string& fragmentSource,
                                  const std::string& versionPreamble,
                                  const std::string& geometrySource,
                                  const std::string& tessControlSource,
                                  const std::string& tessEvaluationSource) {
 ShaderCompileRequest request;
 request.compute = compute;
 request.preamble = versionPreamble;
 request.vertex = vertexSource;
 request.fragment = fragmentSource;
 request.geometry = geometrySource;
 request.tessControl = tessControlSource;
 request.tessEvaluation = tessEvaluationSource;
 request.contentHash = ShaderProgram::contentHash(compute, versionPreamble, vertexSource, fragmentSource,
                                                  geometrySource, tessControlSource, tessEvaluationSource,
                                                  render::vertex_abi::abiSaltString());
 return request;
}
} // namespace

ProgramCache::ProgramCache(ShaderCompileService& compiler) : compiler_(compiler) {}

ProgramCache::~ProgramCache() = default;

ShaderProgram* ProgramCache::getFromSource(const std::string& key,
                                           const std::string& vertexSource,
                                           const std::string& fragmentSource,
                                           const std::string& versionPreamble,
                                           const std::string& geometrySource,
                                           const std::string& tessControlSource,
                                           const std::string& tessEvaluationSource) {
 if(const auto found = cache_.find(key); found != cache_.end()) {
  return found->second.failed ? nullptr : found->second.program.get();
 }
 ShaderCompileRequest request =
     buildRequest(false, vertexSource, fragmentSource, versionPreamble, geometrySource, tessControlSource,
                  tessEvaluationSource);
 return compileSync(key, std::move(request), false);
}

ShaderProgram* ProgramCache::getFromComputeSource(const std::string& key,
                                                  const std::string& computeSource,
                                                  const std::string& versionPreamble) {
  if(const auto found = cache_.find(key); found != cache_.end()) {
   return found->second.failed ? nullptr : found->second.program.get();
  }
  ShaderCompileRequest request = buildRequest(true, computeSource, {}, versionPreamble, {}, {}, {});
  return compileSync(key, std::move(request), true);
}

ShaderProgram* ProgramCache::find(const std::string& key) const {
 const auto found = cache_.find(key);
 if(found == cache_.end()) return nullptr;
 return found->second.failed ? nullptr : found->second.program.get();
}

void ProgramCache::clear() {
 cache_.clear();
}

const std::string& ProgramCache::compileError(const std::string& key) const {
 static const std::string empty;
 const auto found = cache_.find(key);
 return found == cache_.end() ? empty : found->second.error;
}

ShaderProgram* ProgramCache::compileSync(const std::string& key,
                                         ShaderCompileRequest request,
                                         bool compute) {
 Entry entry;
 entry.program = std::make_unique<ShaderProgram>();
 ShaderCompileResult result;
 switch(loadBinary(*entry.program, request, result, compiler_)) {
 case CacheBinaryOutcome::Loaded:
  cache_.emplace(key, std::move(entry));
  return cache_.find(key)->second.program.get();
 case CacheBinaryOutcome::Failed:
  entry.error = result.error;
  entry.program.reset();
  entry.failed = true;
  cache_.emplace(key, std::move(entry));
  return nullptr;
 case CacheBinaryOutcome::Unsupported:
  break;
 }
 const bool compiled =
     compute ? entry.program->compileCompute(request.vertex, request.preamble)
             : entry.program->compile(request.vertex, request.fragment, request.preamble, request.geometry,
                                      request.tessControl, request.tessEvaluation);
 if(compiled) {
  ShaderProgram* raw = entry.program.get();
  cache_.emplace(key, std::move(entry));
  return raw;
 }
 entry.error = entry.program->lastError();
 entry.program.reset();
 entry.failed = true;
 cache_.emplace(key, std::move(entry));
 return nullptr;
}
} // namespace net::minecraft::client::gl
