#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace net::minecraft::client::option {
class GameOptions;
}

namespace net::minecraft::client::resource::pack {
class TexturePacks;
}

namespace net::minecraft::client::render::texture {
class DynamicTexture;
}

namespace net::minecraft::client::texture {
class ImageDownload;
class ImageProcessor;
}

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
    [[nodiscard]] const std::vector<int>& getColors(const std::string& path);
    void bindTexture(int id);
    void deleteTexture(int textureId);
    int load(const RasterImage& image);
    void load(const RasterImage& image, int id);
    void addDynamicTexture(net::minecraft::client::render::texture::DynamicTexture* texture);
    void tick();

    int downloadTexture(const std::string& url, const std::string& backup);
    ImageDownload* downloadImage(const std::string& url, ImageProcessor* processor, bool useBetacraftProxy = true);
    void downloadSkinImage(const std::string& url);
    void downloadCapeImage(const std::string& url);
    void releaseImage(const std::string& url);

    [[nodiscard]] static std::filesystem::path resolveResourcePath(const std::string& path);
    [[nodiscard]] static RasterImage loadRasterFromFile(const std::filesystem::path& filePath);
    [[nodiscard]] static RasterImage loadRasterFromBytes(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] static RasterImage loadRasterFromUrl(const std::string& url, bool useBetacraftProxy = true);

    // Mirror of TextureManager fields used to select texture filtering/wrap.
    static bool MIPMAP;
    static bool MIPMAP_LINEAR;
    bool clamp = false;
    bool blur = false;

private:
    void ensureMissingTexture();
    [[nodiscard]] RasterImage loadRasterForResource(const std::string& resourcePath);

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
};

} // namespace net::minecraft::client::texture
