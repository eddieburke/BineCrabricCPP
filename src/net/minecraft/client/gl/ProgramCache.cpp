#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include <utility>
namespace net::minecraft::client::gl {

ProgramCache::ProgramCache(std::filesystem::path cacheDirectory)
    : binaryCache_(std::make_shared<ShaderBinaryCache>(std::move(cacheDirectory))) {}

ProgramCache::ProgramCache(std::shared_ptr<ShaderBinaryCache> binaryCache)
    : binaryCache_(std::move(binaryCache)) {
 if(binaryCache_ == nullptr) binaryCache_ = std::make_shared<ShaderBinaryCache>(std::filesystem::path{});
}

ProgramCache::~ProgramCache() = default;

ShaderProgram* ProgramCache::getFromSource(const std::string& key,
                                           const std::string& vertexSource,
                                           const std::string& fragmentSource,
                                           const std::string& versionPreamble,
                                           const std::string& geometrySource,
                                           const std::string& tessControlSource,
                                           const std::string& tessEvaluationSource) {
 if(const auto found = cache_.find(key); found != cache_.end()) {
  ++stats_.memoryHits;
  return found->second.failed ? nullptr : found->second.program.get();
 }
 const std::uint64_t contentHash = ShaderProgram::contentHash(
     false, versionPreamble, vertexSource, fragmentSource, geometrySource, tessControlSource,
     tessEvaluationSource, render::vertex_abi::abiSaltString());
 if(ShaderProgram* program = loadBinary(key, contentHash)) return program;
 if(cache_.contains(key)) return nullptr;
 return compileFromSource(key, contentHash, vertexSource, fragmentSource, versionPreamble,
                          geometrySource, tessControlSource, tessEvaluationSource);
}

ShaderProgram* ProgramCache::getFromComputeSource(const std::string& key,
                                                  const std::string& computeSource,
                                                  const std::string& versionPreamble) {
  if(const auto found = cache_.find(key); found != cache_.end()) {
   ++stats_.memoryHits;
   return found->second.failed ? nullptr : found->second.program.get();
  }
  const std::uint64_t contentHash = ShaderProgram::contentHash(
      true, versionPreamble, computeSource, {}, {}, {}, {}, render::vertex_abi::abiSaltString());
  if(ShaderProgram* program = loadBinary(key, contentHash)) return program;
  if(cache_.contains(key)) return nullptr;
  return compileFromComputeSource(key, contentHash, computeSource, versionPreamble);
}

ShaderProgram* ProgramCache::loadBinary(const std::string& key, std::uint64_t contentHash) {
 if(const auto found = cache_.find(key); found != cache_.end()) {
  ++stats_.memoryHits;
  return found->second.failed ? nullptr : found->second.program.get();
 }
 const std::optional<ProgramBinaryBlob> binary = binaryCache_->tryLoad(contentHash);
 if(!binary) {
  ++stats_.binaryMisses;
  return nullptr;
 }
 Entry entry;
 entry.program = std::make_unique<ShaderProgram>();
 if(!entry.program->loadFromBinary(*binary)) {
  ++stats_.binaryRejects;
  binaryCache_->remove(contentHash);
  return nullptr;
 }
 ++stats_.binaryHits;
 ShaderProgram* raw = entry.program.get();
 cache_.emplace(key, std::move(entry));
 return raw;
}

ShaderProgram* ProgramCache::compileFromSource(const std::string& key,
                                               std::uint64_t contentHash,
                                               const std::string& vertexSource,
                                               const std::string& fragmentSource,
                                               const std::string& versionPreamble,
                                               const std::string& geometrySource,
                                               const std::string& tessControlSource,
                                               const std::string& tessEvaluationSource) {
 return compileSync(key, false, vertexSource, fragmentSource, versionPreamble, geometrySource,
                    tessControlSource, tessEvaluationSource, contentHash);
}

ShaderProgram* ProgramCache::compileFromComputeSource(const std::string& key,
                                                      std::uint64_t contentHash,
                                                      const std::string& computeSource,
                                                      const std::string& versionPreamble) {
 return compileSync(key, true, computeSource, {}, versionPreamble, {}, {}, {}, contentHash);
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
                                         bool compute,
                                         const std::string& vertexSource,
                                         const std::string& fragmentSource,
                                         const std::string& versionPreamble,
                                         const std::string& geometrySource,
                                         const std::string& tessControlSource,
                                         const std::string& tessEvaluationSource,
                                         std::uint64_t contentHash) {
 if(const auto found = cache_.find(key); found != cache_.end()) {
  ++stats_.memoryHits;
  return found->second.failed ? nullptr : found->second.program.get();
 }
 Entry entry;
 entry.program = std::make_unique<ShaderProgram>();
 ++stats_.sourceCompiles;
 ProgramBinaryBlob binary;
 if(compute) {
  entry.program->compileComputeToBinary(binary, vertexSource, versionPreamble);
 } else {
  entry.program->compileToBinary(binary, vertexSource, fragmentSource, versionPreamble,
                                 geometrySource, tessControlSource, tessEvaluationSource);
 }
 if(entry.program->valid()) {
  if(!binary.bytes.empty()) {
   binary.contentHash = contentHash;
   binaryCache_->storeAsync(std::move(binary));
   ++stats_.binaryStores;
  }
  ShaderProgram* raw = entry.program.get();
  cache_.emplace(key, std::move(entry));
  return raw;
 }
 entry.error = entry.program->lastError();
 entry.program.reset();
 entry.failed = true;
 ++stats_.compileFailures;
 cache_.emplace(key, std::move(entry));
 return nullptr;
}
} // namespace net::minecraft::client::gl
