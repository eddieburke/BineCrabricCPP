#pragma once
// ProgramCache — compiles and memoizes ShaderProgram instances. Shaderpack programs
// fetch through here so a setting change that alters the define set recompiles
// lazily on the next request.
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
class ProgramCache {
 public:
 ProgramCache() = default;
 // Compiles and memoizes from source strings. Returns nullptr if compilation failed;
 // the failure is cached so we do not thrash the compiler every frame.
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
 std::unordered_map<std::string, Entry> cache_;
};
} // namespace net::minecraft::client::gl
