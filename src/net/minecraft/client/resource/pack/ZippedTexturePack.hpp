#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "net/minecraft/client/resource/pack/TexturePack.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::resource::pack {
class ZippedTexturePack : public TexturePack {
public:
  struct ZipEntry {
    std::string name;
    std::uint32_t localHeaderOffset = 0;
    std::uint32_t compressedSize = 0;
    std::uint32_t uncompressedSize = 0;
    std::uint16_t compressionMethod = 0;
  };
  explicit ZippedTexturePack(std::filesystem::path file, const TexturePack* fallbackResources = nullptr);
  ~ZippedTexturePack() override;
  void open() override;
  void close() override;
  void load() override;
  void unload(texture::TextureManager& textureManager) override;
  void bindIcon(texture::TextureManager& textureManager) override;
  [[nodiscard]] std::vector<std::uint8_t> getResource(std::string_view path) const override;
  [[nodiscard]] const std::filesystem::path& file() const noexcept {
    return file_;
  }

private:
  std::filesystem::path file_;
  const TexturePack* fallbackResources_ = nullptr;
  std::vector<std::uint8_t> archive_;
  std::vector<ZipEntry> entries_;
  std::optional<texture::RasterImage> icon_;
  int iconId_ = -1;
};
} // namespace net::minecraft::client::resource::pack
