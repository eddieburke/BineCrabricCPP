#pragma once
#include <optional>
#include "net/minecraft/client/resource/ResourcePack.hpp"
#include "net/minecraft/client/resource/pack/TexturePack.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
namespace net::minecraft::client::resource::pack {
class BuiltInTexturePack : public TexturePack {
 public:
 explicit BuiltInTexturePack(const ResourcePack& resources);
 void unload(texture::TextureManager& textureManager) override;
 void bindIcon(texture::TextureManager& textureManager) override;
 [[nodiscard]] std::vector<std::uint8_t> getResource(std::string_view path) const override;

 private:
 const ResourcePack& resources_;
 std::optional<texture::RasterImage> icon_;
 int iconId_ = -1;
};
} // namespace net::minecraft::client::resource::pack
