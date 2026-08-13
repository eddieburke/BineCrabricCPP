#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
}
namespace net::minecraft::client::texture {
class ImageDownload;
class SkinImageProcessor;
} // namespace net::minecraft::client::texture
namespace net::minecraft::client::texture {
struct RasterImage {
 int width = 0;
 int height = 0;
 std::vector<std::uint32_t> argb;
};
class TextureManager {
 public:
 explicit TextureManager(option::GameOptions* options = nullptr);
 ~TextureManager();
 void setTexturePacks(resource::pack::TexturePacks* texturePacks);
 void reload();
 [[nodiscard]] int getTextureId(const std::string& path);
 [[nodiscard]] int getTextureId(const std::string& path, const RasterImage& image);
 [[nodiscard]] bool getTextureDimensions(const std::string& path, int& outWidth, int& outHeight);
 [[nodiscard]] bool getTextureDimensionsForId(int textureId, int& outWidth, int& outHeight) const;
 [[nodiscard]] bool isGridAtlasTexture(int textureId) const;
 [[nodiscard]] std::string getCompanionTexturePath(int textureId, std::string_view suffix) const;
 [[nodiscard]] bool isMissingTextureId(int textureId) const noexcept {
  return textureId == missingTextureId_;
 }
 [[nodiscard]] bool resourceExists(const std::string& path) const;
 [[nodiscard]] const std::vector<int>& getColors(const std::string& path);
 void bindTexture(int id);
 void deleteTexture(int textureId);
 int load(const RasterImage& image);
 void load(const RasterImage& image, int id);
 void update(int id, const RasterImage& image);
 void addDynamicTexture(net::minecraft::client::render::texture::DynamicTexture* texture);
 void tick();
 int downloadTexture(const std::string& url, const std::string& backup = "");
 void downloadSkinImage(const std::string& url);
 void downloadCapeImage(const std::string& url);
 void releaseImage(const std::string& url);
 [[nodiscard]] std::optional<bool> skinSlimArms(const std::string& url) const;
 [[nodiscard]] static std::filesystem::path resolveResourcePath(const std::string& path);
 [[nodiscard]] static std::vector<std::uint8_t> rasterToRgba(const RasterImage& image);
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
 static bool MIPMAP;
 static bool MIPMAP_LINEAR;
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/gl/shader/StandardMacros.java:50
 static int MIPMAP_LEVEL;

 private:
 ImageDownload* downloadImage(const std::string& url, SkinImageProcessor* processor,
                              bool useBetacraftProxy = true);
 void ensureMissingTexture();
 option::GameOptions* gameOptions_ = nullptr;
 resource::pack::TexturePacks* texturePacks_ = nullptr;
 std::unordered_map<std::string, int> textures_;
 std::unordered_map<int, std::string> texturePaths_;
 std::unordered_map<int, std::array<int, 2>> textureDimensions_;
 std::unordered_map<std::string, std::vector<int>> colors_;
 std::unordered_map<int, RasterImage> images_;
 std::unordered_map<std::string, std::unique_ptr<ImageDownload>> downloadedImages_;
 std::vector<net::minecraft::client::render::texture::DynamicTexture*> dynamicTextures_;
 int missingTextureId_ = 0;
 bool missingTextureReady_ = false;
 bool clamp = false;
 bool blur = false;
};
} // namespace net::minecraft::client::texture
