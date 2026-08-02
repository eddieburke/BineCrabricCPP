#pragma once
// ProgramCache — compiles and memoizes ShaderProgram instances. Shaderpack programs
// fetch through here so a setting change that alters the define set recompiles
// lazily on the next request.
//
// Compilation is asynchronous when the ShaderCompileService is running (the real
// client): getFromSource submits the request to a worker and returns nullptr until
// poll() (called once per frame) links the finished binary into this cache on the
// main thread. That keeps pack switches / setting changes from stalling the render
// loop. When the service is not started (headless/test runs) the same calls fall
// back to a synchronous compile so behavior is unchanged.
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
class ProgramCache {
 public:
 ProgramCache() = default;
 ~ProgramCache();
 ProgramCache(const ProgramCache&) = delete;
 ProgramCache& operator=(const ProgramCache&) = delete;
 // Compiles and memoizes from source strings. Returns nullptr if compilation failed
 // (failure is cached so we do not thrash the compiler every frame) or if the
 // compile is still in flight on a worker (call poll() next frame).
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
 // Kick off an asynchronous compile without waiting. Used by pack prewarm to start
 // compiling every enabled program the moment a pack loads, so the first frame that
 // needs a program usually finds it cached (or completes within a frame).
 std::uint64_t submit(const std::string& key,
                      const std::string& vertexSource,
                      const std::string& fragmentSource,
                      const std::string& versionPreamble,
                      const std::string& geometrySource = {},
                      const std::string& tessControlSource = {},
                      const std::string& tessEvaluationSource = {});
 std::uint64_t submitCompute(const std::string& key,
                             const std::string& computeSource,
                             const std::string& versionPreamble);
 // True while `key` has a compile in flight (submitted, binary not linked yet).
 // PackCompiler uses this to suppress "failed to compile" logging for programs that
 // are merely still building.
 [[nodiscard]] bool isPending(const std::string& key) const;
 [[nodiscard]] bool hasPending() const noexcept {
  return !pending_.empty();
 }
 void cancelPending();
 // Link every finished worker binary into this cache on the main thread (GL context
 // must be current). Returns true if at least one program became ready.
 bool poll();
 // Drops every cached program (called on shaderpack reload). Programs are deleted.
 void clear();
 [[nodiscard]] std::size_t size() const {
  return cache_.size();
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
 // Synchronous fallback used when the compile service is not running (tests /
 // headless). Mirrors the pre-async behavior exactly.
 ShaderProgram* getFromSourceBlocking(const std::string& key,
                                      ShaderCompileRequest request,
                                      bool compute);
 // Mark a key as in-flight; returns the content hash (0 means already resolved).
 std::uint64_t markPending(const std::string& key, ShaderCompileRequest request);
 void releasePending();

 std::unordered_map<std::string, Entry> cache_;
 // keys with a worker compile in flight -> content hash. poll() resolves these.
 std::unordered_map<std::string, std::uint64_t> pending_;
};
} // namespace net::minecraft::client::gl
