#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace net::minecraft::util::math {
struct Matrix4f;
}
namespace net::minecraft::client::gl {
class ShaderProgram {
 public:
 ShaderProgram() = default;
 ~ShaderProgram();
 ShaderProgram(const ShaderProgram&) = delete;
 ShaderProgram& operator=(const ShaderProgram&) = delete;
 ShaderProgram(ShaderProgram&& other) noexcept;
 ShaderProgram& operator=(ShaderProgram&& other) noexcept;
 bool compile(const std::string& vertexSource,
              const std::string& fragmentSource,
              const std::string& versionPreamble,
              const std::string& geometrySource = {},
              const std::string& tessControlSource = {},
              const std::string& tessEvaluationSource = {});
 bool compileCompute(const std::string& computeSource,
                     const std::string& versionPreamble);
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
 // Iris /* RENDERTARGETS: a,b */ → glDrawBuffers(COLOR_ATTACHMENT0+a, ...).
 // Empty = leave DrawBuffers unchanged (composite write FBOs remap attachments).
 void setDrawBufferColortexIndices(const std::vector<int>& colortexIndices);
 void applyDrawBuffers(int colorAttachmentCount = 8) const;
 int location(std::string_view name) const;
 enum class SamplerKind {
  None,
  Float,
  Integer,
  Unsigned,
  Shadow,
  Volume
 };
 [[nodiscard]] SamplerKind samplerKind(std::string_view name) const;
 [[nodiscard]] const std::vector<std::string>& declaredSamplers() const;
 [[nodiscard]] bool legacyAttributes() const {
  return legacyAttributes_;
 }
 [[nodiscard]] bool tessellation() const {
  return tessellation_;
 }
 void set1iAt(int location, int value) const;
 void set1fAt(int location, float value) const;
 void set2fAt(int location, const float* values) const;
 void set3fAt(int location, const float* values) const;
 void set4fAt(int location, const float* values) const;
 void set2iAt(int location, const int* values) const;
 void set3iAt(int location, const int* values) const;
 void set4iAt(int location, const int* values) const;
 void setMatrix3At(int location, const float* values) const;
 void setMatrix4At(int location, const float* values) const;
 void set1i(std::string_view name, int value) const;
 void set1f(std::string_view name, float value) const;
 void set2f(std::string_view name, float x, float y) const;
 void set3f(std::string_view name, float x, float y, float z) const;
 void set4f(std::string_view name, float x, float y, float z, float w) const;
 void setMatrix3(std::string_view name, const float* value, bool transpose = false) const;
 void setMatrix4(std::string_view name, const float* value, bool transpose = false) const;
 void setMatrix4(std::string_view name, const net::minecraft::util::math::Matrix4f& value) const;
 [[nodiscard]] static bool supported();

 private:
 struct TransparentStringHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view value) const noexcept {
   return std::hash<std::string_view>{}(value);
  }
 };
 template <typename Value>
 using NameMap = std::unordered_map<std::string, Value, TransparentStringHash, std::equal_to<>>;
 void reflectSamplers();
 unsigned int program_ = 0;
 mutable NameMap<int> uniformCache_;
 NameMap<SamplerKind> samplerKinds_;
 std::vector<std::string> samplerNames_;
 bool legacyAttributes_ = false;
 bool tessellation_ = false;
 std::vector<unsigned int> drawBuffers_{};
 std::string lastError_;
};
} // namespace net::minecraft::client::gl
