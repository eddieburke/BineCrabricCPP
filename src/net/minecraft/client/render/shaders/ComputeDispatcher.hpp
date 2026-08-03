#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
class PackInstance;
class ColorTargets;
struct PackUniformValues;
struct PackViewportValues;
namespace ComputeDispatcher {
[[nodiscard]] inline bool matchesStage(std::string_view name, std::string_view stage) {
 if(!name.starts_with(stage)) {
  return false;
 }
 std::size_t offset = stage.size();
 if(offset < name.size() && std::isdigit(static_cast<unsigned char>(name[offset])) != 0) {
  if(stage == "final" || name[offset] == '0') {
   return false;
  }
  int suffix = 0;
  while(offset < name.size() && std::isdigit(static_cast<unsigned char>(name[offset])) != 0) {
   suffix = suffix * 10 + static_cast<int>(name[offset] - '0');
   if(suffix > 99) {
    return false;
   }
   ++offset;
  }
 }
 if(offset == name.size()) {
  return true;
 }
 return offset + 2 == name.size() && name[offset] == '_' &&
        name[offset + 1] >= 'a' && name[offset + 1] <= 'z';
}
[[nodiscard]] inline std::array<unsigned int, 3> workGroups(const PackPass& pass, int width, int height) {
 if(!pass.relativeGroups) {
  return {static_cast<unsigned int>(std::max(1, pass.groups[0])),
          static_cast<unsigned int>(std::max(1, pass.groups[1])),
          static_cast<unsigned int>(std::max(1, pass.groups[2]))};
 }
 const float lx = static_cast<float>(std::max(1, pass.localSize[0]));
 const float ly = static_cast<float>(std::max(1, pass.localSize[1]));
 return {
     static_cast<unsigned int>(
         std::max(1, static_cast<int>(std::ceil(static_cast<float>(width) * pass.groupScale[0] / lx)))),
     static_cast<unsigned int>(
         std::max(1, static_cast<int>(std::ceil(static_cast<float>(height) * pass.groupScale[1] / ly)))),
     1u};
}
inline constexpr unsigned int kBarrierBits = 0xFFFFFFFFu;
[[nodiscard]] inline bool attachedToPass(const std::string& computeName, const std::string& passName) {
 if(computeName == passName) {
  return true;
 }
 if(computeName.size() != passName.size() + 2 || computeName.rfind(passName + "_", 0) != 0) {
  return false;
 }
 const char suffix = computeName.back();
 return suffix >= 'a' && suffix <= 'z';
}
[[nodiscard]] inline int computePassOrder(const std::string& name) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shaderpack/programs/ProgramSet.java
 const std::size_t under = name.rfind('_');
 const bool hasLetter = under != std::string::npos && under + 2 == name.size() &&
                        name.back() >= 'a' && name.back() <= 'z';
 const std::string_view base = hasLetter ? std::string_view(name).substr(0, under) : std::string_view(name);
 std::size_t digitStart = base.size();
 while(digitStart > 0 && std::isdigit(static_cast<unsigned char>(base[digitStart - 1])) != 0) --digitStart;
 int index = 0;
 if(digitStart < base.size()) {
  index = std::atoi(std::string(base.substr(digitStart)).c_str());
 }
 const int letter = hasLetter ? name.back() - 'a' + 1 : 0;
 return index * 27 + letter;
}
[[nodiscard]] inline bool lessComputeOrder(const PackPass& a, const PackPass& b) {
 if(a.order != b.order) {
  return a.order < b.order;
 }
 const int sa = computePassOrder(a.name);
 const int sb = computePassOrder(b.name);
 if(sa != sb) {
  return sa < sb;
 }
 return a.name < b.name;
}
[[nodiscard]] inline std::string computeParentName(const std::string& name) {
 const std::size_t under = name.rfind('_');
 if(under != std::string::npos && under + 2 == name.size() && name.back() >= 'a' && name.back() <= 'z') {
  return name.substr(0, name.size() - 2);
 }
 return name;
}
[[nodiscard]] inline bool lessComputeParent(const std::string& a, const std::string& b) {
 auto split = [](const std::string& name) -> std::pair<std::string, int> {
  std::size_t i = name.size();
  while(i > 0 && std::isdigit(static_cast<unsigned char>(name[i - 1])) != 0) {
   --i;
  }
  if(i == name.size()) {
   return {name, -1};
  }
  return {name.substr(0, i), std::atoi(name.c_str() + i)};
 };
 const auto [ap, an] = split(a);
 const auto [bp, bn] = split(b);
 if(ap != bp) {
  return ap < bp;
 }
 return an < bn;
}
bool dispatch(PackInstance& pack,
              const PackPass& pass,
              const PackUniformValues& uniforms,
              const PackViewportValues* viewport,
              std::unordered_map<std::string, int>& textures,
              std::unordered_map<std::string, int>& colorImages,
              std::unordered_map<std::string, int>& volumes,
              const ColorTargets* colorTargets,
              int width,
              int height,
              bool barrier,
              const std::function<gl::ShaderProgram*(PackInstance&, const std::string&)>& compile);
} // namespace ComputeDispatcher
} // namespace net::minecraft::client::render
