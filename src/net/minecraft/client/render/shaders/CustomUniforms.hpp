#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
enum class CustomUniformType {
 Float,
 Int,
 Bool,
 Vec2,
 Vec3,
 Vec4
};
struct CustomUniformDecl {
 std::string name;
 CustomUniformType type = CustomUniformType::Float;
 bool upload = true;
 std::string expression;
};
struct CustomUniformValue {
 CustomUniformType type = CustomUniformType::Float;
 float f[4]{};
 int i = 0;
 bool b = false;
 [[nodiscard]] float asFloat() const noexcept;
 [[nodiscard]] int asInt() const noexcept;
 [[nodiscard]] bool asBool() const noexcept;
};
class CustomUniformRuntime {
 public:
 struct SmoothState {
  float value = 0.0f;
  bool initialized = false;
 };
 CustomUniformRuntime();
 ~CustomUniformRuntime();
 CustomUniformRuntime(CustomUniformRuntime&&) noexcept;
 CustomUniformRuntime& operator=(CustomUniformRuntime&&) noexcept;
 CustomUniformRuntime(const CustomUniformRuntime&) = delete;
 CustomUniformRuntime& operator=(const CustomUniformRuntime&) = delete;
 void clear();
 bool compile(const std::vector<CustomUniformDecl>& decls, std::string& error);
 void setOptions(std::unordered_map<std::string, std::string> options);
 void evaluate(const PackUniformValues& frame);
 void upload(const gl::ShaderProgram& program) const;
 [[nodiscard]] const std::unordered_map<std::string, CustomUniformValue>& values() const noexcept {
  return values_;
 }

 private:
 struct Compiled;
 std::vector<std::unique_ptr<Compiled>> compiled_;
 std::unordered_map<std::string, CustomUniformValue> values_;
 std::unordered_map<std::string, std::string> options_;
 std::unordered_map<int, SmoothState> smoothStates_;
 int autoSmoothId_ = 100000;
};
[[nodiscard]] bool parseCustomUniformType(std::string_view text, CustomUniformType& out) noexcept;
[[nodiscard]] const char* customUniformTypeName(CustomUniformType type) noexcept;
} // namespace net::minecraft::client::render
