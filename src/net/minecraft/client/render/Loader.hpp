#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
namespace net::minecraft::client::render::shaderpack {
enum class ShaderOptionForm {
 Define,
 Constant
};
struct ShaderSourceOption {
 PackSetting setting;
 ShaderOptionForm form = ShaderOptionForm::Define;
};
class ShaderPackLoader {
 public:
 using ReadText = std::function<std::string(std::string_view)>;
 static bool load(const std::vector<std::string>& resources,
                  const ReadText& readText,
                  ShaderPackDefinition& out,
                  std::unordered_map<std::string, ShaderSourceOption>& options,
                  std::string& error);
 static std::string rewriteOptions(const std::string& source,
                                   const std::unordered_map<std::string, ShaderSourceOption>& options,
                                   const std::unordered_map<std::string, std::string>& values);
};
} // namespace net::minecraft::client::render::shaderpack
