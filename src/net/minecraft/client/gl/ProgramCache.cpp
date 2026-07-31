#include "net/minecraft/client/gl/ProgramCache.hpp"
namespace net::minecraft::client::gl {
ShaderProgram* ProgramCache::getFromSource(const std::string& key,
                                           const std::string& vertexSource,
                                           const std::string& fragmentSource,
                                           const std::string& versionPreamble,
                                           const std::string& geometrySource,
                                           const std::string& tessControlSource,
                                           const std::string& tessEvaluationSource) {
 const auto found = cache_.find(key);
 if(found != cache_.end()) {
  return found->second.failed ? nullptr : found->second.program.get();
 }
 Entry entry;
 entry.program = std::make_unique<ShaderProgram>();
 if(!entry.program->compile(vertexSource, fragmentSource, versionPreamble, geometrySource, tessControlSource,
                            tessEvaluationSource)) {
  entry.error = entry.program->lastError();
  entry.program.reset();
  entry.failed = true;
  cache_.emplace(key, std::move(entry));
  return nullptr;
 }
 ShaderProgram* raw = entry.program.get();
 cache_.emplace(key, std::move(entry));
 return raw;
}
ShaderProgram* ProgramCache::getFromComputeSource(const std::string& key,
                                                  const std::string& computeSource,
                                                  const std::string& versionPreamble) {
 const auto found = cache_.find(key);
 if(found != cache_.end()) {
  return found->second.failed ? nullptr : found->second.program.get();
 }
 Entry entry;
 entry.program = std::make_unique<ShaderProgram>();
 if(!entry.program->compileCompute(computeSource, versionPreamble)) {
  entry.error = entry.program->lastError();
  entry.program.reset();
  entry.failed = true;
  cache_.emplace(key, std::move(entry));
  return nullptr;
 }
 ShaderProgram* raw = entry.program.get();
 cache_.emplace(key, std::move(entry));
 return raw;
}
const std::string& ProgramCache::compileError(const std::string& key) const {
 static const std::string empty;
 const auto found = cache_.find(key);
 return found == cache_.end() ? empty : found->second.error;
}
void ProgramCache::clear() {
 cache_.clear();
}
} // namespace net::minecraft::client::gl
