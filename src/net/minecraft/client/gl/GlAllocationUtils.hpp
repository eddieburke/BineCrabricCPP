#pragma once
#include <mutex>
#include <vector>
#include "net/minecraft/client/gl/GlState.hpp"
namespace net::minecraft::client::gl {
class GlAllocationUtils {
public:
  static void generateTextureName(unsigned int& outName) {
    std::lock_guard lock(mutex());
    gl::genTextures(1, &outName);
    textureNames().push_back(outName);
  }
  static void clear() {
    std::lock_guard lock(mutex());
    auto& names = textureNames();
    if(!names.empty()) {
      gl::deleteTextures(static_cast<int>(names.size()), names.data());
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
} // namespace net::minecraft::client::gl
