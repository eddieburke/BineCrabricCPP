#pragma once
#include "net/minecraft/client/gl/GL11.hpp"
#include <mutex>
#include <vector>
namespace net::minecraft::client::util {
class GlAllocationUtils {
public:
  static void generateTextureName(unsigned int& outName) {
    std::lock_guard lock(mutex());
    gl::GL11::glGenTextures(1, &outName);
    textureNames().push_back(outName);
  }
  static void clear() {
    std::lock_guard lock(mutex());
    auto& names = textureNames();
    if(!names.empty()) {
      gl::GL11::glDeleteTextures(static_cast<int>(names.size()), names.data());
    }
    names.clear();
  }

private:
  static std::mutex& mutex() {
    static std::mutex m;
    return m;
  }
  static std::vector<unsigned int>& textureNames() {
    static std::vector<unsigned int> v;
    return v;
  }
};
} // namespace net::minecraft::client::util
