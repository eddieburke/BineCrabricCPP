#pragma once
#include <array>
#include <cstdint>
#include "net/minecraft/entity/EntityTypes.hpp"
namespace net::minecraft {
}
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::texture {
class TextureManager;
}
namespace net::minecraft::map {
class MapState;
}
namespace net::minecraft::client::render {
// Faithful port of net.minecraft.client.render.MapRenderer (beta 1.7.3 MCP).
class MapRenderer {
public:
  MapRenderer() = default;
  MapRenderer(font::TextRenderer* textRendererIn, net::minecraft::client::texture::TextureManager* textureManagerIn);
  void render(net::minecraft::PlayerEntity& player,
              net::minecraft::client::texture::TextureManager& textureManagerIn,
              const net::minecraft::map::MapState& mapState);

private:
  std::array<std::int32_t, 16384> colors_{};
  int texture_ = 0;
  font::TextRenderer* textRenderer_ = nullptr;
};
} // namespace net::minecraft::client::render
