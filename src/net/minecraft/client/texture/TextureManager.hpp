#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft {
class World;
}
namespace net::minecraft::client::resource::pack {
class TexturePacks;
}
namespace net::minecraft::client::render::texture {
class DynamicTexture;
} // namespace net::minecraft::client::render::texture
namespace net::minecraft::client::texture {
class ImageDownload;
class ImageProcessor;
} // namespace net::minecraft::client::texture
namespace net::minecraft::client::texture {
struct RasterImage {
 int width = 0;
 int height = 0;
 std::vector<std::uint32_t> argb;
};
// Faithful subset of net.minecraft.client.texture.TextureManager (beta 1.7.3).
class TextureManager {
 public:
 explicit TextureManager(option::GameOptions* options = nullptr);
 ~TextureManager();
 void setTexturePacks(resource::pack::TexturePacks* texturePacks);
 void reload();
  [[nodiscard]] int getTextureId(const std::string& path);
  [[nodiscard]] int getCustomTextureGlId(int textureId);
  [[nodiscard]] bool getTextureDimensions(const std::string& path, int& outWidth, int& outHeight);
  [[nodiscard]] bool resourceExists(const std::string& path) const;
 [[nodiscard]] const std::vector<int>& getColors(const std::string& path);
 void bindTexture(int id);
 void bindTextureOrAtlas(int textureId, const std::string& defaultAtlasPath);
 void deleteTexture(int textureId);
 int load(const RasterImage& image);
 void load(const RasterImage& image, int id);
 void update(int id, const RasterImage& image);
 void addDynamicTexture(net::minecraft::client::render::texture::DynamicTexture* texture);
 void tick();
 int downloadTexture(const std::string& url, const std::string& backup);
 ImageDownload* downloadImage(const std::string& url, ImageProcessor* processor, bool useBetacraftProxy = true);
 void downloadSkinImage(const std::string& url);
 void downloadCapeImage(const std::string& url);
 void releaseImage(const std::string& url);
 [[nodiscard]] std::optional<bool> skinSlimArms(const std::string& url) const;
 [[nodiscard]] static std::filesystem::path resolveResourcePath(const std::string& path);
 [[nodiscard]] static RasterImage loadRasterFromFile(const std::filesystem::path& filePath);
 [[nodiscard]] static RasterImage loadRasterFromBytes(const std::vector<std::uint8_t>& bytes);
 [[nodiscard]] static RasterImage loadRasterFromUrl(const std::string& url, bool useBetacraftProxy = true);
 [[nodiscard]] RasterImage loadRasterForResource(const std::string& resourcePath);
 [[nodiscard]] const RasterImage* getRasterImage(int textureId) const {
  const auto it = images_.find(textureId);
  if(it != images_.end()) {
   return &it->second;
  }
  return nullptr;
 }
 // Mirror of TextureManager fields used to select texture filtering/wrap.
 static bool MIPMAP;
 static bool MIPMAP_LINEAR;
 bool clamp = false;
 bool blur = false;

 private:
 void ensureMissingTexture();
 option::GameOptions* gameOptions_ = nullptr;
 resource::pack::TexturePacks* texturePacks_ = nullptr;
 std::unordered_map<std::string, int> textures_;
 std::unordered_map<std::string, std::vector<int>> colors_;
 // CPU copies for textures loaded without a pack path (font, pack icons, etc.).
 // Pack-path textures reload from the active texture pack via textures_.
 std::unordered_map<int, RasterImage> images_;
 std::unordered_map<std::string, std::unique_ptr<ImageDownload>> downloadedImages_;
 std::vector<net::minecraft::client::render::texture::DynamicTexture*> dynamicTextures_;
 int missingTextureId_ = 0;
 bool missingTextureReady_ = false;
 std::unordered_set<std::string> missingTextureWarned_;
};
} // namespace net::minecraft::client::texture
