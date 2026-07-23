#pragma once
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::render::shaderpack {
enum class SettingType {
 Int,
 Bool,
 Float
};
struct PackSetting {
 std::string key;
 SettingType type = SettingType::Bool;
 std::string label;
 double minimum = 0.0;
 double maximum = 1.0;
 double step = 0.01;
 std::string defaultValue;
};
struct PackProgram {
 std::string stage;
 std::string vertex;
 std::string fragment;
};
struct PackTarget {
 std::string format = "RGBA8";
 float scale = 1.0f;
};
struct PackPass {
 std::string name;
 std::string type;
 std::string program;
 std::vector<std::string> inputs;
 std::string output;
};
class PackManifest {
 public:
 static bool parse(const std::string& text, PackManifest& out, std::string& error);
 [[nodiscard]] bool valid() const noexcept {
  return valid_;
 }
 std::string name = "Unnamed shaderpack";
 std::string version = "1.0";
 std::vector<PackSetting> settings;
 std::unordered_map<std::string, PackProgram> programs;
 std::unordered_map<std::string, PackTarget> targets;
 std::vector<PackPass> passes;

 private:
 bool valid_ = false;
};
} // namespace net::minecraft::client::render::shaderpack
