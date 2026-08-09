#pragma once
// ProgramCache — compiles and memoizes ShaderProgram instances. Shaderpack programs
// fetch through here so a setting change that alters the define set recompiles
// lazily on the next request.
//
// Compilation is synchronous on the calling (render) thread, mirroring Iris: all
// GL work happens on the render thread at controlled points (pack load, prewarm,
// first use), and the driver parallelizes internally via
// glMaxShaderCompilerThreadsKHR. The earlier design compiled on worker threads
// sharing the primary GL context; it deadlocked the NVIDIA driver at startup and
// world load, so there is no async path anymore.
//
// Owns the on-disk program binary cache directly: a disk hit is one
// glProgramBinary link, a disk miss is one glLinkProgram compile (its binary is
// then extracted and stored for next time). Never both for the same program.
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
struct ProgramCacheStats {
 std::uint64_t memoryHits = 0;
 std::uint64_t binaryHits = 0;
 std::uint64_t binaryMisses = 0;
 std::uint64_t binaryRejects = 0;
 std::uint64_t sourceCompiles = 0;
 std::uint64_t compileFailures = 0;
 std::uint64_t binaryStores = 0;
};
class ProgramCache {
 public:
 explicit ProgramCache(std::filesystem::path cacheDirectory = {});
 explicit ProgramCache(std::shared_ptr<ShaderBinaryCache> binaryCache);
 ~ProgramCache();
 ProgramCache(const ProgramCache&) = delete;
 ProgramCache& operator=(const ProgramCache&) = delete;
 // Compiles (if not cached) and memoizes from source strings. Returns nullptr only
 // if compilation failed (failure is cached so we do not thrash the compiler every
 // frame).
 //
 // `key` alone identifies the entry — the preamble and defines are not folded into
 // it. A caller that changes either must clear() first, which is what a shaderpack
 // setting change does.
  ShaderProgram* getFromSource(const std::string& key,
                               const std::string& vertexSource,
                               const std::string& fragmentSource,
                               const std::string& versionPreamble,
                               const std::string& geometrySource = {},
                               const std::string& tessControlSource = {},
                               const std::string& tessEvaluationSource = {});
  ShaderProgram* getFromComputeSource(const std::string& key,
                                      const std::string& computeSource,
                                      const std::string& versionPreamble);
  ShaderProgram* loadBinary(const std::string& key, std::uint64_t contentHash);
  ShaderProgram* compileFromSource(const std::string& key,
                                   std::uint64_t contentHash,
                                   const std::string& vertexSource,
                                   const std::string& fragmentSource,
                                   const std::string& versionPreamble,
                                   const std::string& geometrySource = {},
                                   const std::string& tessControlSource = {},
                                   const std::string& tessEvaluationSource = {});
  ShaderProgram* compileFromComputeSource(const std::string& key,
                                          std::uint64_t contentHash,
                                          const std::string& computeSource,
                                          const std::string& versionPreamble);
  // Cached program for `key`, or nullptr when it is unknown or failed. Unlike
  // getFromSource this never compiles, so a caller that already knows the key can
  // skip preparing the sources again.
  [[nodiscard]] ShaderProgram* find(const std::string& key) const;
  // Drops every cached program (called on shaderpack reload). Programs are deleted.
  void clear();
 [[nodiscard]] std::size_t size() const {
  return cache_.size();
 }
 [[nodiscard]] const ProgramCacheStats& stats() const noexcept {
  return stats_;
 }
 // GL info log from the most recent failed compile, kept per key so a caller can
 // report why a pack went dark. Empty when that key compiled, or is unknown.
 [[nodiscard]] const std::string& compileError(const std::string& key) const;

 private:
 struct Entry {
  std::unique_ptr<ShaderProgram> program; // null if this key failed to compile
  bool failed = false;
  std::string error; // GL info log, populated only when failed
 };
 // Compiles on the current (render) thread, GL context must be current, and
 // caches the resulting program (or the failure) under `key`. `vertexSource`
 // holds the compute source when `compute` is true.
 ShaderProgram* compileSync(const std::string& key,
                            bool compute,
                            const std::string& vertexSource,
                            const std::string& fragmentSource,
                            const std::string& versionPreamble,
                            const std::string& geometrySource,
                            const std::string& tessControlSource,
                            const std::string& tessEvaluationSource,
                            std::uint64_t contentHash);

 std::shared_ptr<ShaderBinaryCache> binaryCache_;
 std::unordered_map<std::string, Entry> cache_;
 ProgramCacheStats stats_{};
};
} // namespace net::minecraft::client::gl
