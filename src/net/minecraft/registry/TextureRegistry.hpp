#pragma once
#include <string>
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::registry {
class TextureRegistry {
 public:
 static constexpr int kCustomTextureBase = 256;
 struct Entry {
  std::string path;
  int glId = -1;
  int width = 0;
  int height = 0;
 };
 static int getOrRegisterTexture(const std::string& path);
 static const std::string& getTexturePath(int textureId);
 static bool isCustomTexture(int textureId) noexcept;
 static const Entry* getEntry(int textureId);
 static int resolveGlId(int textureId, net::minecraft::client::texture::TextureManager& textureManager);
 static void seedResolvedTexture(int textureId, int glId, int width, int height);
 static void invalidateGlIds();
};
} // namespace net::minecraft::registry
