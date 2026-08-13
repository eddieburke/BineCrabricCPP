#pragma once
#include <optional>
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
 static bool isCustomTexture(int textureId) noexcept;
 static std::optional<Entry> getEntry(int textureId);
 static int resolveGlId(int textureId, net::minecraft::client::texture::TextureManager& textureManager);
 static void seedResolvedTexture(int textureId, int glId, int width, int height);
 static int releaseTexture(int textureId);
 static void invalidateGlIds();
};
} // namespace net::minecraft::registry
