#pragma once
// ShaderProgram — GLSL compile/link with #define injection, uniform-location cache,
// and typed setters. Backs the uniform-driven pipeline (Phase B) and the standalone
// shaderpack system (Phase D). Version preamble is supplied by the caller so the same
// source can target GLSL 120 (Phase B compat) or 330 core (Phase C+).
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::util::math {
struct Matrix4f;
}
namespace net::minecraft::client::gl {
// A single key=value define injected into both stages before compilation. Value may be
// empty for a bare `#define NAME`.
struct ShaderDefine {
  std::string name;
  std::string value;
};
class ShaderProgram {
public:
  ShaderProgram() = default;
  ~ShaderProgram();
  ShaderProgram(const ShaderProgram&) = delete;
  ShaderProgram& operator=(const ShaderProgram&) = delete;
  ShaderProgram(ShaderProgram&& other) noexcept;
  ShaderProgram& operator=(ShaderProgram&& other) noexcept;
  // Compiles + links. `versionPreamble` is emitted verbatim as the first line(s)
  // (e.g. "#version 120\n" or "#version 330 core\n"). Returns false on failure; the
  // GL info log is captured in lastError().
  bool compile(const std::string& vertexSource,
               const std::string& fragmentSource,
               const std::string& versionPreamble,
               const std::vector<ShaderDefine>& defines = {});
  void destroy();
  [[nodiscard]] bool valid() const {
    return program_ != 0;
  }
  [[nodiscard]] unsigned int handle() const {
    return program_;
  }
  const std::string& lastError() const {
    return lastError_;
  }
  void bind() const;
  static void unbind();
  // Uniform setters. Location lookups are cached by name; unknown names are cached as
  // -1 and become no-ops, so callers can set uniforms unconditionally.
  int location(const std::string& name) const;
  void set1i(const std::string& name, int value) const;
  void set1f(const std::string& name, float value) const;
  void set2f(const std::string& name, float x, float y) const;
  void set3f(const std::string& name, float x, float y, float z) const;
  void set4f(const std::string& name, float x, float y, float z, float w) const;
  void setMatrix4(const std::string& name, const float* value, bool transpose = false) const;
  void setMatrix4(const std::string& name, const net::minecraft::util::math::Matrix4f& value) const;
  // True only if the driver exposes every entry point the class relies on.
  [[nodiscard]] static bool supported();

private:
  unsigned int program_ = 0;
  mutable std::unordered_map<std::string, int> uniformCache_;
  std::string lastError_;
};
} // namespace net::minecraft::client::gl
