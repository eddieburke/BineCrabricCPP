#pragma once
// ProgramCache — compiles and memoizes ShaderProgram instances keyed by
// (vertex path, fragment path, version preamble, define set). Both the engine
// ubershader (Phase B) and shaderpack programs (Phase D) fetch through here so a
// setting change that alters the define set recompiles lazily on next get().
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
class ProgramCache {
 public:
 ProgramCache() = default;
 explicit ProgramCache(std::string resourceRoot);
 // Returns a compiled program, or nullptr if compilation failed (error printed and
 // cached so we do not thrash the compiler each frame). `vshPath`/`fshPath` are
 // relative to the resource root. `versionPreamble` selects GLSL 120 vs 330 core.
 ShaderProgram* get(const std::string& vshPath,
                    const std::string& fshPath,
                    const std::string& versionPreamble,
                    const std::vector<ShaderDefine>& defines = {});
 // Compile directly from source strings (used for embedded/generated shaders).
 ShaderProgram* getFromSource(const std::string& key,
                              const std::string& vertexSource,
                              const std::string& fragmentSource,
                              const std::string& versionPreamble,
                              const std::vector<ShaderDefine>& defines = {});
 // Drops every cached program (called on shaderpack reload). Programs are deleted.
 void clear();
 [[nodiscard]] std::size_t size() const {
  return cache_.size();
 }
 // Loads a resource-relative text file; empty string if missing.
 std::string loadText(const std::string& relPath) const;

 private:
 struct Entry {
  std::unique_ptr<ShaderProgram> program; // null if this key failed to compile
  bool failed = false;
 };
 static std::string makeKey(const std::string& a,
                            const std::string& b,
                            const std::string& versionPreamble,
                            const std::vector<ShaderDefine>& defines);
 std::string resourceRoot_;
 std::unordered_map<std::string, Entry> cache_;
};
} // namespace net::minecraft::client::gl
