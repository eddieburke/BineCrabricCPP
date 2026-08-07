#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::client::render {
enum class PackOptionForm {
 Define,
 Constant
};
struct PackSourceOption {
 PackSetting setting;
 PackOptionForm form = PackOptionForm::Define;
};
// Preprocesses shaders.properties/dimension.properties text: evaluates its
// #if/#ifdef/#elif/#else/#endif with the option table seeded from `values`
// (falling back to each option's default). Exported for the pack tests.
std::string preprocessProperties(const std::string& source,
                                 int mcVersion,
                                 const std::unordered_map<std::string, PackSourceOption>& options,
                                 const std::unordered_map<std::string, std::string>& values);
class PackLoader {
 public:
 using ReadText = std::function<std::string(std::string_view)>;
 static bool load(const std::vector<std::string>& resources,
                  const ReadText& readText,
                  PackDefinition& out,
                  std::unordered_map<std::string, PackSourceOption>& options,
                  std::string& error,
                  const std::unordered_map<std::string, std::string>& values = {});
 static std::string rewriteOptions(const std::string& source,
                                   const std::unordered_map<std::string, PackSourceOption>& options,
                                   const std::unordered_map<std::string, std::string>& values);
};
} // namespace net::minecraft::client::render
